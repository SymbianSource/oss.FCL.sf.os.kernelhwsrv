// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\nkernsa\fastbuf.cpp
// 
//

#include <nktest/nkutils.h>

template <class T>
class WaitFreePipe
	{
public:
	static WaitFreePipe<T>* New(TInt aSize);
	~WaitFreePipe();
	void InitReader();
	void InitWriter();
	void Read(T& aOut);
	TInt Write(const T& aIn);
	inline TUint32 Waits() {return iWaits;}
	inline void ResetWaits() {iWaits = 0;}
private:
	WaitFreePipe();
private:
	T* volatile iWrite;
	T* volatile iRead;
	T* iBase;
	T* iEnd;
	NRequestStatus* volatile iStat;
	NThread* iReader;
	volatile TUint32 iWaits;
	};

template <class T>
WaitFreePipe<T>::WaitFreePipe()
	:	iStat(0),
		iReader(0),
		iWaits(0)
	{
	}

template <class T>
WaitFreePipe<T>::~WaitFreePipe()
	{
	free(iBase);
	}

template <class T>
WaitFreePipe<T>* WaitFreePipe<T>::New(TInt aSize)
	{
	WaitFreePipe<T>* p = new WaitFreePipe<T>;
	if (!p)
		return 0;
	p->iBase = (T*)malloc(aSize * sizeof(T));
	if (!p->iBase)
		{
		delete p;
		return 0;
		}
	p->iEnd = p->iBase + aSize;
	p->iWrite = p->iBase;
	p->iRead = p->iBase;
	return p;
	}

template <class T>
void WaitFreePipe<T>::InitWriter()
	{
	}

template <class T>
void WaitFreePipe<T>::InitReader()
	{
	iReader = NKern::CurrentThread();
	}

template <class T>
void WaitFreePipe<T>::Read(T& aOut)
	{
	while (iRead == iWrite)
		{
		NRequestStatus s;
		s = KRequestPending;
		// make sure set to KRequestPending is seen before iStat write
		__e32_atomic_store_ord_ptr(&iStat, &s);
		// make sure writer sees our request status before we check for buffer empty again
		if (iRead != iWrite)
			RequestComplete(iReader, (NRequestStatus*&)iStat, 0);
		WaitForRequest(s);
		++iWaits;
		}
	aOut = *iRead;
	T* new_read = iRead + 1;
	if (new_read == iEnd)
		new_read = iBase;
	// make sure read of data value is observed before update of read pointer
	__e32_atomic_store_rel_ptr(&iRead, new_read);
	}

template <class T>
TInt WaitFreePipe<T>::Write(const T& aIn)
	{
	T* new_write = iWrite + 1;
	if (new_write == iEnd)
		new_write = iBase;
	if (new_write == iRead)
		return KErrOverflow;	// buffer full
	*iWrite = aIn;
	// make sure data is seen before updated write pointer
	__e32_atomic_store_ord_ptr(&iWrite, new_write);
	if (iStat)
		RequestComplete(iReader, (NRequestStatus*&)iStat, 0);
	return KErrNone;
	}


struct SPipeTest
	{
	WaitFreePipe<TUint32>* iPipe;
	TUint64 iTotalWrites;
	TUint64 iTotalReads;
	volatile TUint32 iWrites;
	volatile TUint32 iReads;
	TUint32 iMeasure;
	volatile TUint32 iReadTime;
	volatile TUint32 iWriteTime;
	volatile TBool iStop;
	};

void PipeWriterThread(TAny* aPtr)
	{
	SPipeTest& a = *(SPipeTest*)aPtr;
	a.iPipe->InitWriter();
	TUint32 seed[2] = {1,0};
	TUint32 seqs[2] = {3,0};
	TInt r;
	while (!a.iStop)
		{
		TUint32 x = random(seqs);
		do	{
			r = a.iPipe->Write(x);
			if (r != KErrNone)
				fcfspin(2*a.iWriteTime);
			} while (r != KErrNone);
		++a.iTotalWrites;
		++a.iWrites;
		while (a.iWrites>=a.iMeasure)
			{}
		TUint32 time = random(seed) % a.iWriteTime;
		fcfspin(time);
		}
	}

void PipeReaderThread(TAny* aPtr)
	{
	SPipeTest& a = *(SPipeTest*)aPtr;
	TUint32 seed[2] = {2,0};
	TUint32 seqs[2] = {3,0};
	a.iPipe->InitReader();
	a.iPipe->ResetWaits();
	while (!a.iStop)
		{
		TUint32 x = random(seqs);
		TUint32 y;
		a.iPipe->Read(y);
		TEST_RESULT(x==y, "Wrong value");
		++a.iTotalReads;
		++a.iReads;
		if (a.iReads < a.iMeasure)
			{
			TUint32 time = random(seed) % a.iReadTime;
			fcfspin(time);
			continue;
			}
		TUint32 w = a.iPipe->Waits();
		TUint32 wr = (w<<4)/a.iMeasure;
		TEST_PRINT3("%d waits out of %d (wr=%d)", w, a.iMeasure, wr);
		TUint32 rt = a.iReadTime;
		TUint32 wt = a.iWriteTime;
		switch (wr)
			{
			case 0:
				a.iReadTime = rt>>1;
				a.iWriteTime = wt<<1;
				break;
			case 1:
			case 2:
			case 3:
				a.iReadTime = rt - (rt>>2);
				a.iWriteTime = wt + (wt>>2);
				break;
			case 4:
			case 5:
			case 6:
				a.iReadTime = rt - (rt>>3);
				a.iWriteTime = wt + (wt>>3);
				break;
			case 7:
			case 8:
				// ok
				break;
			case 9:
			case 10:
			case 11:
				a.iReadTime = rt - (rt>>3);
				a.iWriteTime = wt + (wt>>3);
				break;
			case 12:
			case 13:
			case 14:
				a.iReadTime = rt + (rt>>2);
				a.iWriteTime = wt - (wt>>2);
				break;
			case 15:
			case 16:
				a.iReadTime = rt<<1;
				a.iWriteTime = wt>>1;
				break;
			}
		TEST_PRINT4("RT: %d->%d WT: %d->%d", rt, a.iReadTime, wt, a.iWriteTime);
		a.iPipe->ResetWaits();
		a.iReads = 0;
		a.iWrites = 0;
		}
	}

void DoPipeTest()
	{
	SPipeTest a;
	memclr(&a, sizeof(a));
	a.iPipe = WaitFreePipe<TUint32>::New(1024);
	TEST_OOM(a.iPipe);
	a.iMeasure = 131072;
	a.iReadTime = 1024;
	a.iWriteTime = 1024;

	NFastSemaphore exitSem(0);
	NThread* reader = CreateThreadSignalOnExit("Reader", &PipeReaderThread, 11, &a, 0, -1, &exitSem, 0);
	TEST_OOM(reader);
	NThread* writer = CreateThreadSignalOnExit("Writer", &PipeWriterThread, 11, &a, 0, -1, &exitSem, 1);
	TEST_OOM(writer);

	while (a.iTotalWrites < 0x01000000u)
		NKern::Sleep(1000);
	a.iStop = TRUE;

	NKern::FSWait(&exitSem);
	NKern::FSWait(&exitSem);
	}

void TestWaitFreePipe()
	{
	DoPipeTest();
	}


