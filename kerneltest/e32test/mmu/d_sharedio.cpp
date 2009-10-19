// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\d_sharedio.cpp
// LDD for testing SharedIoBuffers
// 
//

#include <kernel/kern_priv.h>
#include "d_sharedio.h"

//
// LDD factory
//

class DSharedIoTestFactory : public DLogicalDevice
	{
public:
	DSharedIoTestFactory();
	~DSharedIoTestFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
public:
	DSharedIoBuffer* iGlobalBuffer;
	};

//
// Logical Channel
//

class DSharedIoTest : public DLogicalChannelBase
	{
public:
	virtual ~DSharedIoTest();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
public:
	DSharedIoBuffer* iIoBuffer;
	DSharedIoBuffer* iGlobalBuffer;
	DSharedIoTestFactory* iFactory;
#ifndef __WINS__
	TPhysAddr iPhysAddress;
#endif
	};

//
// LDD factory
//

DSharedIoTestFactory::DSharedIoTestFactory()
	{
	iGlobalBuffer=NULL;
	}

DSharedIoTestFactory::~DSharedIoTestFactory()
	{
	delete iGlobalBuffer;
	}
TInt DSharedIoTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
	__KTRACE_OPT(KMMU,Kern::Printf(">DSharedIoTestFactory::Create iGlobalBuffer=%x",iGlobalBuffer));
	if(!iGlobalBuffer)
		{
#ifdef __WINS__
		TUint aAttribs=0;
#else
		TUint aAttribs=EMapAttrSupRw | EMapAttrFullyBlocking;
#endif
		TInt r=DSharedIoBuffer::New(iGlobalBuffer,KSizeGlobalBuffer,aAttribs);
		if(r!=KErrNone)
			return r;
		}
	aChannel=new DSharedIoTest;
	if(!aChannel)
		return KErrNoMemory;
	((DSharedIoTest*)aChannel)->iGlobalBuffer=iGlobalBuffer;
	((DSharedIoTest*)aChannel)->iFactory=this;
	__KTRACE_OPT(KMMU,Kern::Printf("<DSharedIoTestFactory::Create iGlobalBuffer=%x",iGlobalBuffer));
	return KErrNone;
	}

TInt DSharedIoTestFactory::Install()
	{
	return SetName(&KSharedIoTestLddName);
	}

void DSharedIoTestFactory::GetCaps(TDes8& /* aDes */) const
	{
	//aDes.FillZ(aDes.MaxLength());
	}

DECLARE_STANDARD_LDD()
	{
	return new DSharedIoTestFactory;
	}

//
// Logical Channel
//

TInt DSharedIoTest::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

DSharedIoTest::~DSharedIoTest()
	{
	delete iIoBuffer;
	}

TInt checkBuffer(TAny* buffer, TUint32 aSize, TUint32 key)
	{
	TInt r=KErrNone;
	TUint8* m=(TUint8*)buffer;
	for(TUint32 size=0;size<aSize;size++,key+=5,m++)
		{
		if(*m!=(TUint8)(key%256))
			{
			r=KErrCorrupt;
			break;
			}
		}
	return r;
	}

TInt fillBuffer(TAny* buffer, TUint32 aSize, TUint32 key)
	{
	TUint8* m=(TUint8*)buffer;
	for(TUint32 size=0;size<aSize;size++,key+=5,m++)
		{
		*m=(TUint8)(key%256);
		}
	return KErrNone;
	}

static void AppendNumToBuf(TDes8& aDes, const TDesC& aNum, TInt width, char fill)
{
	TInt l = aNum.Length();
	for (; l < width; ++l)
		aDes.Append(TChar(fill));
	aDes.Append(aNum);
}

static void DumpMemory(TUint8* aStart, TInt aSize)
{
	TBuf8<80> line;
	TBuf8<24> val;
	TChar space(' ');

	TInt i = (TInt)aStart & 0xF;	// first byte in this line to dump
	TInt n = 16;					// end byte in this line

	while (aSize > 0)
		{
		if (i + aSize < 16)
			n = i + aSize;

		val.Num((TUint32)aStart & ~0xF, EHex);
		AppendNumToBuf(line, val, 8, '0');
		line.Append(space);
		line.Append(space);

		TInt j;

		for (j = 0; j < i; ++j)
			{
			line.Append(space);
			line.Append(space);
			line.Append(space);

			if (j == 7) line.Append(space);
			}

		for (; j < n; ++j)
			{
			val.Num(aStart[j-i], EHex);
			line.Append(space);
			AppendNumToBuf(line, val, 2, '0');

			if (j == 7) line.Append(space);
			}

		for (; j < 16; ++j)
			{
			line.Append(space);
			line.Append(space);
			line.Append(space);

			if (j == 7) line.Append(space);
			}

		line.Append(space);
		line.Append(space);

		for (j = 0; j < i; ++j)
			line.Append(space);

		for (; j < n; ++j)
			{
			char c = aStart[j-i];
			if (c < ' ' || c > 126) c = '.';
			line.Append(TChar(c));
			}

		Kern::Printf("%S", &line);

		line.SetLength(0);

		aStart += (n - i);
		aSize -= (n - i);

		i = 0;
		}
}

TBool CheckMemCleared(TLinAddr aAddress, TInt aSize)
{
	TUint8* aPtr = (TUint8*)aAddress;
	for(TInt i = 0; i<aSize; i++)
		{
		if(aPtr[i] != 0x03)
			{
			Kern::Printf("CheckMemCleared(0x%x, %d) failed at i = 0x%x", aAddress, aSize, i);
			// Start on current line & ~0xF, run for 16 lines x 16 bytes
			TUint8 *p = (TUint8*)((aAddress + i) & ~0x0F);
			TInt n = 256;

			if (p < aPtr) p = aPtr;
			if (p - aPtr > aSize - n)	// if (p + n > aPtr + aSize) rearranged (to avoid overflow)
				n = aPtr + aSize - p;

			DumpMemory(p, n);
			return EFalse;
			}
		}
	return ETrue;
}

TInt DSharedIoTest::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNone;
	switch (aFunction)
		{
		case RTestLdd::ECreateBuffer:
			{
			TUint32 size = (TUint32)a1;
			r = KErrNoMemory;
#ifdef __WINS__
			TUint aAttribs1=0;
			TUint aAttribs2=0;
			TUint aAttribs3=0;
#else
			TUint aAttribs1=EMapAttrSupRw | EMapAttrBufferedNC;
			TUint aAttribs2=EMapAttrSupRw | EMapAttrFullyBlocking;
			TUint aAttribs3=EMapAttrSupRw | EMapAttrCachedMax;
#endif
			NKern::ThreadEnterCS();
			r=DSharedIoBuffer::New(iIoBuffer,size,aAttribs1);
			if(r!=KErrNone)
				{
				Kern::Printf("Error creating buffer r=%d\n",r);
				NKern::ThreadLeaveCS();
				return r;
				}

			//Check the buffer is properly initialized (the previous content 
			//deleted by inserting all 0x03s)
			if (!CheckMemCleared(iIoBuffer->iAddress, iIoBuffer->iSize))
				{
				Kern::Printf("Error memory zeroing test for shared io buffers");
				NKern::ThreadLeaveCS();
				return KErrCorrupt;
				}

			//just test that we can construct a second shared buffer
			DSharedIoBuffer* ptr;
			r=DSharedIoBuffer::New(ptr,size,aAttribs2);
			if(r!=KErrNone)
				{
				Kern::Printf("Error creating the 2nd buffer r=%d\n",r);
				delete iIoBuffer;
				iIoBuffer=NULL;
				NKern::ThreadLeaveCS();
				return r;
				}
			delete ptr; //creation successfull, simply delete the object

			// and the third one, this time fully cached.
			r=DSharedIoBuffer::New(ptr,size,aAttribs3);
			if(r!=KErrNone)
				{
				Kern::Printf("Error creating the 3rd buffer r=%d\n",r);
				delete iIoBuffer;
				iIoBuffer=NULL;
				NKern::ThreadLeaveCS();
				return r;
				}
			delete ptr; //creation successfull, simply delete the object

			NKern::ThreadLeaveCS();
			if(iIoBuffer->iSize!=size)   // test
				{
				Kern::Printf("Error checking size iIoBuffer->iSize=%d size=%d\n",iIoBuffer->iSize,size);
				return KErrGeneral;
				}
			memset((void*)iIoBuffer->iAddress,0,size);
			}
			return r;

		case RTestLdd::EMapInGlobalBuffer:
			{
			if(!iGlobalBuffer)
				return KErrGeneral;

			TUint id;
			kumemget32(&id,a1,sizeof(TUint));

			NKern::ThreadEnterCS();
			Kern::Containers()[EProcess]->Wait();
			DProcess* process=Kern::ProcessFromId(id);
			if(process)
				process->Open();
			Kern::Containers()[EProcess]->Signal();
			if(process)
				{
				r=iGlobalBuffer->UserMap(process);
				process->Close(0);
				}
			else
				r = KErrGeneral;
			NKern::ThreadLeaveCS();

			if(r!=KErrNone)
				return r;

			if(iGlobalBuffer->UserToKernel(iGlobalBuffer->iUserAddress,iGlobalBuffer->iSize)!=iGlobalBuffer->iAddress)
				return KErrGeneral;
			
			if(iGlobalBuffer->UserToKernel(iGlobalBuffer->iUserAddress,iGlobalBuffer->iSize+1)!=NULL)
				return KErrGeneral;

			if(iGlobalBuffer->KernelToUser(iGlobalBuffer->iAddress)!=iGlobalBuffer->iUserAddress)
				return KErrGeneral;

			kumemput32(a1,&iGlobalBuffer->iUserAddress,sizeof(TAny*));
			kumemput32(a2,&iGlobalBuffer->iSize,sizeof(TInt));

			return KErrNone;
			}

		case RTestLdd::EMapOutGlobalBuffer:
			{
			if(!iGlobalBuffer)
				return KErrGeneral;
			r=iGlobalBuffer->UserUnmap();
			if(r==KErrNone)
				if(iGlobalBuffer->iUserProcess)
					r = KErrGeneral;
			return r;
			}

		case RTestLdd::EDestroyGlobalBuffer:
			{
			NKern::ThreadEnterCS();
			delete iGlobalBuffer;
			iGlobalBuffer = NULL;
			iFactory->iGlobalBuffer=NULL;
			NKern::ThreadLeaveCS();
			return KErrNone;
			}

		case RTestLdd::ECreateBufferPhysAddr:
			{
#ifdef __WINS__
			return KErrNotSupported;
#else
			TUint32 size=Kern::RoundToPageSize(1);
			NKern::ThreadEnterCS();
			r=Epoc::AllocPhysicalRam(size,iPhysAddress);
			Kern::Printf("phys addr = %X!\n",iPhysAddress);
			if(r!=KErrNone)
				{
				NKern::ThreadLeaveCS();
				return r;
				}
			r = KErrNoMemory;

			//test that we can construct a fully cached sharedio
			DSharedIoBuffer* ptr;
			r=DSharedIoBuffer::New(ptr,iPhysAddress,size,EMapAttrSupRw|EMapAttrCachedMax);
			if(r!=KErrNone)
				{
				Kern::Printf("Error creating the physical cached buffer r=%d\n",r);
				Epoc::FreePhysicalRam(iPhysAddress,size);
				iPhysAddress=0;
				NKern::ThreadLeaveCS();
				return r;
				}
			delete ptr; //creation successfull, simply delete the object


			r=DSharedIoBuffer::New(iIoBuffer,iPhysAddress,size,EMapAttrSupRw|EMapAttrFullyBlocking);
			if(r!=KErrNone)
				{
				Epoc::FreePhysicalRam(iPhysAddress,size);
				iPhysAddress=0;
				NKern::ThreadLeaveCS();
				return r;
				}

			if(iIoBuffer->iSize!=size)   // test
				{
				delete iIoBuffer;
				iIoBuffer=NULL;
				Epoc::FreePhysicalRam(iPhysAddress,size);
				iPhysAddress=0;
				NKern::ThreadLeaveCS();
				return KErrGeneral;
				}

			fillBuffer((TAny*)iIoBuffer->iAddress,size,180);

			DPlatChunkHw* hwChunk;
			r=DPlatChunkHw::New(hwChunk, iPhysAddress, size, EMapAttrSupRw|EMapAttrFullyBlocking);
			if(r!=KErrNone)
				{
				delete iIoBuffer;
				iIoBuffer=NULL;
				Epoc::FreePhysicalRam(iPhysAddress,size);
				iPhysAddress=0;
				NKern::ThreadLeaveCS();
				return r;
				}
			
			r=checkBuffer((TAny*)hwChunk->LinearAddress(),size,180);
			if(r!=KErrNone)
				{
				delete iIoBuffer;
				iIoBuffer=NULL;
				hwChunk->Close(NULL);
				Epoc::FreePhysicalRam(iPhysAddress,size);
				iPhysAddress=0;
				NKern::ThreadLeaveCS();
				return r;
				}

			hwChunk->Close(NULL);
			NKern::ThreadLeaveCS();
			return r;
#endif
			}

		case RTestLdd::EDestroyBufferPhysAddr:
			{
#ifdef __WINS__
			return KErrNotSupported;
#else
			TUint32 size=Kern::RoundToPageSize(1);
			NKern::ThreadEnterCS();
			delete iIoBuffer;
			iIoBuffer = NULL;
			r=Epoc::FreePhysicalRam(iPhysAddress,size);
			iPhysAddress=0;
			NKern::ThreadLeaveCS();
			return r;
#endif
			}


		case RTestLdd::EMapInBuffer:
			{
			r=iIoBuffer->UserMap(&Kern::CurrentProcess());
			if(r!=KErrNone)
				return r;

			TAny** p = (TAny**)iIoBuffer->iAddress;
			TAny* ua = (TAny*)iIoBuffer->iUserAddress;
			TAny** end = (TAny**)((TInt)p+iIoBuffer->iSize);
			while(p<end)
				{
				*p++ = ua;
				ua = (TAny*)((TInt)ua+sizeof(TAny*));
				}
			if(iIoBuffer->UserToKernel(iIoBuffer->iUserAddress,iIoBuffer->iSize)!=iIoBuffer->iAddress)
				return KErrGeneral;
			
			if(iIoBuffer->UserToKernel(iIoBuffer->iUserAddress,iIoBuffer->iSize+1)!=NULL)
				return KErrGeneral;

			if(iIoBuffer->KernelToUser(iIoBuffer->iAddress)!=iIoBuffer->iUserAddress)
				return KErrGeneral;
			kumemput32(a1,&iIoBuffer->iUserAddress,sizeof(TAny*));
			kumemput32(a2,&iIoBuffer->iSize,sizeof(TInt));
			return r;
			}

		case RTestLdd::EMapOutBuffer:
			{
			r=iIoBuffer->UserUnmap();
			if(r==KErrNone)
				if(iIoBuffer->iUserProcess)
					r = KErrGeneral;
			return r;
			}

		case RTestLdd::EDestroyBuffer:
			NKern::ThreadEnterCS();
			delete iIoBuffer;
			iIoBuffer = NULL;
			NKern::ThreadLeaveCS();
			return KErrNone;

		case RTestLdd::ECheckBuffer:
			if(!iIoBuffer->iAddress || !iIoBuffer->iUserAddress || !iIoBuffer->iUserProcess)
				return KErrGeneral;
			return checkBuffer((TAny*)iIoBuffer->iAddress,iIoBuffer->iSize,(TUint32)a1);
			
		case RTestLdd::EFillBuffer:
			if(!iIoBuffer->iAddress || !iIoBuffer->iUserAddress || !iIoBuffer->iUserProcess)
				return KErrGeneral;
			return fillBuffer((TAny*)iIoBuffer->iAddress,iIoBuffer->iSize,(TUint32)a1);

		case RTestLdd::EThreadRW:
			{
			TInt dummy;
			TPckg<TInt> a(dummy);
			DThread* pT;
			if((TInt)a2==-1)
				{
				pT=&Kern::CurrentThread();
				}
			else
				{
				NKern::ThreadEnterCS();
				DObjectCon* pC=Kern::Containers()[EThread];
				pC->Wait();
				pT=Kern::ThreadFromId((TInt)a2);
				pC->Signal();
				if(!pT)
					return KErrNotFound;
				NKern::ThreadLeaveCS();
				}
			r=Kern::ThreadDesRead(pT,a1,a,0,KChunkShiftBy0);
			if(r!=KErrNone)
				return r;
			if(dummy!=KMagic1)
				return KErrCorrupt;
			dummy=KMagic2;
			r=Kern::ThreadDesWrite(pT,a1,a,0,KChunkShiftBy0,&Kern::CurrentThread());
			if(r!=KErrNone)
				return r;
			return KErrNone;
			}

		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}

