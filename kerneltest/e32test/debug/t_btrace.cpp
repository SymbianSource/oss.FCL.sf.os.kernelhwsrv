// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\debug\t_btrace.cpp
// Overview:
// Tests generation of traces using the BTrace APIs
// and captuirng of these by BTRACE.LDD and BTRACEC.DLL.
// API Information:
// class BTrace
// class RBTrace
// BTrace0
// BTrace4
// BTrace8
// BTrace12
// BTraceN
// BTraceBig
// BTracePc0
// BTracePc4
// BTracePc8
// BTracePc12
// BTracePcN
// BTracePcBig
// BTraceContext0
// BTraceContext4
// BTraceContext8
// BTraceContext12
// BTraceContextN
// BTraceContextBig
// BTraceContextPc0
// BTraceContextPc4
// BTraceContextPc8
// BTraceContextPc12
// BTraceContextPcN
// BTraceContextPcBig
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include <e32def.h>
#include <e32def_private.h>

#include "../../../kernel/eka/include/d32btrace.h"
#include "../../../kernel/eka/include/e32btrace.h"
#include "d_btrace.h"

#define __TRACE_LINE__()	test.Printf(_L("%d\n"),__LINE__)

RTest test(_L("T_BTRACE"));

TUint BaseSize;		// Size of a standard test trace with no data

RBTrace Trace;
RBTraceTest TraceTest;

TInt ContextOffset(const TUint8* aData)
	{
	TInt size = 4; // header size
	if(aData[BTrace::EFlagsIndex]&BTrace::EHeader2Present)
		size += 4;
	if(aData[BTrace::EFlagsIndex]&BTrace::ETimestampPresent)
		size += 4;
	if(aData[BTrace::EFlagsIndex]&BTrace::ETimestamp2Present)
		size += 4;
	return size;
	}


TInt ExtraSize(const TUint8* aData)
	{
	TInt size = ContextOffset(aData);
	if(aData[BTrace::EFlagsIndex]&BTrace::EContextIdPresent)
		size += 4;
	if(aData[BTrace::EFlagsIndex]&BTrace::EPcPresent)
		size += 4;
	if(aData[BTrace::EFlagsIndex]&BTrace::EExtraPresent)
		size += 4;
	return size;
	}


TUint32* Body(const TUint8* aData)
	{
	return (TUint32*)(aData+ExtraSize(aData));
	}


TPtrC8 Text(const TUint8* aData)
	{
	TInt size = aData[BTrace::ESizeIndex];
	TInt extra = ExtraSize(aData);
	extra += 8; // skip past first 2 32bit args
	size -= extra;
	return TPtrC8(aData+extra,size);
	}


const TUint KTest1SubCategory = 0x81;
const TUint KTest2SubCategory = 0xc3;

TUint8 KTestTrace1[KMaxBTraceRecordSize*2] = { BTrace::ETest1, KTest1SubCategory };
TUint8 KTestTrace2[KMaxBTraceRecordSize*2] = { BTrace::ETest2, KTest2SubCategory };
TUint32 BigFilter2[KNumBTraceFilterTestUids];

void Trace1(TInt aSize, TInt aDelay=0)
	{
	test_KErrNone(TraceTest.Trace(0,KTestTrace1,aSize,aDelay));
	}

void Trace2(TInt aSize, TInt aDelay=0)
	{
	test_KErrNone(TraceTest.Trace(0,KTestTrace2,aSize,aDelay));
	}

TBool CheckTrace1(TUint8* aData, TInt aSize, TInt aSubCategory=KTest1SubCategory)
	{
	if(((aData[BTrace::ESizeIndex]+3)&~3)!=aSize)
		return EFalse;
	if(aData[BTrace::ECategoryIndex]!=BTrace::ETest1)
		return EFalse;
	if(aData[BTrace::ESubCategoryIndex]!=aSubCategory)
		return EFalse;
	TInt extra = ExtraSize(aData);
	aSize = aData[BTrace::ESizeIndex]-extra;
	aData += extra;
	while(--aSize>=0)
		{
		if(((TUint8*)KTestTrace1)[4+aSize]!=aData[aSize])
			return EFalse;
		}
	return ETrue;
	}

TBool CheckTrace2(TUint8* aData, TInt aSize, TInt aSubCategory=KTest2SubCategory)
	{
	if(((aData[BTrace::ESizeIndex]+3)&~3)!=aSize)
		return EFalse;
	if(aData[BTrace::ECategoryIndex]!=BTrace::ETest2)
		return EFalse;
	if(aData[BTrace::ESubCategoryIndex]!=aSubCategory)
		return EFalse;
	TInt extra = ExtraSize(aData);
	aSize = aData[BTrace::ESizeIndex]-extra;
	aData += extra;
	while(--aSize>=0)
		{
		if(((TUint8*)KTestTrace2)[4+aSize]!=aData[aSize])
			return EFalse;
		}
	return ETrue;
	}


TBool CheckSize(const TUint8* aData, TInt aSize, TInt aExpected)
	{
	TInt extra = ExtraSize(aData);
	if(aSize==((extra+aExpected+3)&~3))
		return 1;
	else
		{
		TInt actual_size = aData[0];
		if (aSize > actual_size)
			actual_size = aSize;
		test.Printf(_L("Trace data:\n"));
		TInt i;
		for (i=0; i<actual_size; ++i)
			{
			test.Printf(_L(" %02x"), aData[i]);
			if ((i&15)==15 || i==actual_size-1)
				test.Printf(_L("\n"));
			}
		test.Printf(_L("extra=%d aExp=%d aSize=%d\n"), extra, aExpected, aSize);
		return 0;
		}
	}


TInt Trace1Sequence = 0;
TInt Trace2Sequence = 0;

TUint8* TraceData;
TInt TraceDataSize;

TInt BadTrace(TUint8* aData)
	{
	Trace.SetMode(0);
	TUint8* buffer = Trace.DataChunk().Base();
	test.Printf(_L("BAD TRACE: data=%x buffer=%x (dataRead=%x,%x)\n"),aData,buffer,TraceData,TraceDataSize);
	TUint8* bufferEnd =  buffer+((TUint32*)buffer)[1]; // TBTraceBuffer.iEnd
	while(buffer<bufferEnd)
		{
		RDebug::Printf("%08x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x",
			buffer,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7],
			buffer[8],buffer[9],buffer[10],buffer[11],buffer[12],buffer[13],buffer[14],buffer[15]);
		buffer += 16;
		}
	buffer = Trace.DataChunk().Base();
	TInt size = ((TUint32*)buffer)[9];
	buffer += ((TUint32*)buffer)[8];
	bufferEnd = buffer+size;
	test.Printf(_L("copyBuffer=%x\n"),buffer,0);
	while(buffer<bufferEnd)
		{
		RDebug::Printf("%08x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x",
			buffer,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7],
			buffer[8],buffer[9],buffer[10],buffer[11],buffer[12],buffer[13],buffer[14],buffer[15]);
		buffer += 16;
		}
	return 0;
	}

void CheckTraceData(TUint8* aData, TUint aSize)
	{
	TraceData = aData;
	TraceDataSize = aSize;
	TUint8* end = aData+aSize;
	while(aData<end)
		{
		TUint size = (aData[BTrace::ESizeIndex]+3)&~3;
		if(aData+size>end)
			test(BadTrace(aData));
		TUint subCategory = aData[BTrace::ESubCategoryIndex];
		if(aData[BTrace::EFlagsIndex]&(BTrace::EMissingRecord))
			{
			Trace1Sequence = -1;
			Trace2Sequence = -1;
			}
		if(aData[BTrace::ECategoryIndex]==BTrace::ETest1)
			{
			if(subCategory!=(TUint)Trace1Sequence && Trace1Sequence!=-1)
				{
				test.Printf(_L("Sequence wrong %02x!=%02x\n"),subCategory,Trace1Sequence);
				test(BadTrace(aData));
				}
			if(!CheckTrace1(aData,size,subCategory))
				test(BadTrace(aData));
			Trace1Sequence = subCategory+1;
			Trace1Sequence &= 0xff;
			}
		else
			{
			if(aData[BTrace::ECategoryIndex]!=BTrace::ETest2)
				test(BadTrace(aData));
			if(subCategory!=(TUint)Trace2Sequence && Trace2Sequence!=-1)
				{
				test.Printf(_L("Sequence wrong %02x!=%02x\n"),subCategory,Trace2Sequence);
				test(BadTrace(aData));
				}
			if(!CheckTrace2(aData,size,subCategory))
				test(BadTrace(aData));
			Trace2Sequence = subCategory+1;
			Trace2Sequence &= 0xff;
			}
		aData = BTrace::NextRecord(aData);
		}
	}


void DumpTrace()
	{
	TBuf8<(80+KMaxBTraceDataArray*9/4)*2> buf;
	for(;;)
		{
		TUint8* record;
		TInt dataSize = Trace.GetData(record);
		if(!dataSize)
			break;
		TUint8* end = record+dataSize;
		while(record<end)
			{
			TUint size = record[BTrace::ESizeIndex];
			TUint flags = record[BTrace::EFlagsIndex];
			TUint category = record[BTrace::ECategoryIndex];
			TUint subCategory = record[BTrace::ESubCategoryIndex];
			TUint8* data = record+4;
			size -= 4;

			buf.Zero();
			TUint32 header2 = 0;
			if(flags&(BTrace::EHeader2Present))
				{
				header2 = *(TUint32*)data;
				data += 4;
				size -= 4;
				}

			if((flags&(BTrace::ETimestampPresent|BTrace::ETimestamp2Present))==(BTrace::ETimestampPresent|BTrace::ETimestamp2Present))
				{
				buf.AppendFormat(_L8("time:%08x:%08x "),((TUint32*)data)[1],*(TUint32*)data);
				data += 8;
				size -= 8;
				}
			else if(flags&(BTrace::ETimestampPresent|BTrace::ETimestamp2Present))
				{
				buf.AppendFormat(_L8("time:%08x "),*(TUint32*)data);
				data += 4;
				size -= 4;
				}

			if(flags&(BTrace::EContextIdPresent))
				{
				buf.AppendFormat(_L8("context:%08x "),*(TUint32*)data);
				data += 4;
				size -= 4;
				}
			else
				{
				buf.AppendFormat(_L8("                 "));
				}

			if(flags&(BTrace::EPcPresent))
				{
				buf.AppendFormat(_L8("pc:%08x "),*(TUint32*)data);
				data += 4;
				size -= 4;
				}

			TUint32 extra = 0;
			if(flags&(BTrace::EExtraPresent))
				{
				extra = *(TUint32*)data;
				data += 4;
				size -= 4;
				}

			TUint32 data0 = (size>0) ? *(TUint32*)(data) : 0;
			TUint32 data1 = (size>4) ? *(TUint32*)(data+4) : 0;
			TUint32 data2 = (size>8) ? *(TUint32*)(data+8) : 0;
			TPtrC8 des(0,0);
			if(size>=8)
				des.Set(data+8,size-8);
			switch(category)
				{
			case BTrace::EKernPrintf:
			case BTrace::ERDebugPrintf:
			case BTrace::EPlatsecPrintf:
				{
				if(category==BTrace::EKernPrintf)
					buf.Append(_L8("Kern::Printf "));
				else if(category==BTrace::ERDebugPrintf)
					buf.Append(_L8("RDebug::Printf "));
				else
					buf.Append(_L8("PlatSecPrintf "));
				switch(header2&BTrace::EMultipartFlagMask)
					{
					case BTrace::EMultipartFirst:
						buf.AppendFormat(_L8("seq:%d size:%d thread-id:%d \"%S\""),extra,data0,data1,&des);
						break;
					case BTrace::EMultipartMiddle:
					case BTrace::EMultipartLast:
						buf.AppendFormat(_L8("seq:%d size:%d offset:%d \"%S\""),extra,data0,data1,&des);
						break;
					default:
						des.Set(data+4,size-4);
						buf.AppendFormat(_L8("thread-id:%d \"%S\""),data0,&des);
						break;
						}
				}
				break;

			case BTrace::EThreadIdentification:
				{
				switch(subCategory)
					{
				case BTrace::ENanoThreadCreate:
					buf.AppendFormat(_L8("NanoThreadCreate  thrd:%08x"),data0);
					break;
				case BTrace::ENanoThreadDestroy:
					buf.AppendFormat(_L8("NanoThreadDestroy thrd:%08x"),data0);
					break;
				case BTrace::EThreadCreate:
					buf.AppendFormat(_L8("ThreadCreate      thrd:%08x proc:%08x name:%S"),data0,data1,&des);
					break;
				case BTrace::EThreadDestroy:
					buf.AppendFormat(_L8("ThreadDestroy     thrd:%08x proc:%08x id:%d"),data0,data1,data2);
					break;
				case BTrace::EThreadName:
					buf.AppendFormat(_L8("ThreadName        thrd:%08x proc:%08x name:%S"),data0,data1,&des);
					break;
				case BTrace::EThreadId:
					buf.AppendFormat(_L8("ThreadId          thrd:%08x proc:%08x id:%d"),data0,data1,data2);
					break;
				case BTrace::EProcessName:
					buf.AppendFormat(_L8("ProcessName       thrd:%08x proc:%08x name:%S"),data0,data1,&des);
					break;
					}
				}
				break;

			case BTrace::ECpuUsage:
				{
				switch(subCategory)
					{
				case BTrace::EIrqStart:
					buf.AppendFormat(_L8("IrqStart"));
					break;
				case BTrace::EIrqEnd:
					buf.AppendFormat(_L8("IrqEnd"));
					break;
				case BTrace::EFiqStart:
					buf.AppendFormat(_L8("FiqStart"));
					break;
				case BTrace::EFiqEnd:
					buf.AppendFormat(_L8("FiqEnd"));
					break;
				case BTrace::EIDFCStart:
					buf.AppendFormat(_L8("IDFCStart"));
					break;
				case BTrace::EIDFCEnd:
					buf.AppendFormat(_L8("IDFCEnd"));
					break;
				case BTrace::ENewThreadContext:
					buf.AppendFormat(_L8("NewThreadContext"));
					break;
					}
				break;
				}

			case BTrace::EClientServer:
				{
				switch(subCategory)
					{
				case BTrace::EServerCreate:
					buf.AppendFormat(_L8("EServerCreate     serv:%08x name:%S"),data0,&des);
					break;
				case BTrace::EServerDestroy:
					buf.AppendFormat(_L8("EServerDestroy    serv:%08x"),data0);
					break;
				case BTrace::ESessionAttach:
					buf.AppendFormat(_L8("ESessionAttach    sess:%08x serv:%08x"),data0,data1);
					break;
				case BTrace::ESessionDetach:
					buf.AppendFormat(_L8("ESessionDetach    sess:%08x"),data0);
					break;
				case BTrace::EMessageSend:
					buf.AppendFormat(_L8("EMessageSend      mess:%08x func:%08x sess:%08x"),data0,data1,data2);
					break;
				case BTrace::EMessageReceive:
					buf.AppendFormat(_L8("EMessageReceive   mess:%08x"),data0);
					break;
				case BTrace::EMessageComplete:
					buf.AppendFormat(_L8("EMessageComplete  mess:%08x reas:%08x"),data0,data1);
					break;
					}
				break;
				}

			case BTrace::ERequests:
				{
				switch(subCategory)
					{
				case BTrace::ERequestComplete:
					buf.AppendFormat(_L8("ERequestComplete  thrd:%08x stat:%08x resn:%08x"),data0,data1,data2);
					break;
					}
				break;
				}

			default:
				{
				buf.AppendFormat(_L8("size:%d flags:%02x cat:%d,%d data: "),size,flags,category,subCategory);
				for(TUint i=0; i<size; i+=4)
					buf.AppendFormat(_L8("%08x "),*(TUint32*)(data+i));
				}
				break;
				}
			buf.Append('\r');
			buf.Append('\n');
			RDebug::RawPrint(buf.Expand());

			record = BTrace::NextRecord(record);
			}
		Trace.DataUsed();
		}
	}


//---------------------------------------------
//! @SYMTestCaseID KBASE-T_BTRACE-0058-0059
//! @SYMTestType UT
//! @SYMPREQ PREQ1030
//! @SYMTestCaseDesc Basic functionality tests which run in both 'sample' and 'free-running' modes.
//! @SYMTestActions Test basic functionality provided by the functions:
//!		RBTrace::SetFilter(), RBTrace::Empty(),
//!		RBTrace::GetData(), RBTrace::DataUsed(),
//!		RBTrace::RequestData(), RBTrace::CancelRequestData(),
//!		RBTrace::BufferSize(), and RBTrace::ResizeBuffer()
//! @SYMTestExpectedResults Function produce expected results.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void TestBasics(TUint aMode)
	{
	aMode |= RBTrace::EEnable;

	TUint8* data;
	TInt size;

	test.Start(_L("Check a second Open() fails"));
	RBTrace dummy;
	TInt r = dummy.Open();
	test_Equal(KErrInUse,r);
	r = dummy.Open();
	test_Equal(KErrInUse,r);

	test.Next(_L("Reset trace buffer"));
	Trace.SetMode(0);
	Trace.Empty();
	Trace.SetMode(aMode);

	TUint8* buffer_base = Trace.DataChunk().Base();
	test.Printf(_L("Buffer base %08x\n"), buffer_base);

	test.Next(_L("Test SetFilter() and GetData()"));
	Trace.SetFilter(BTrace::ETest1,0);
	Trace.SetFilter(BTrace::ETest2,0);
	size = Trace.GetData(data);
	test_Equal(0,size);
	Trace1(4);
	size = Trace.GetData(data);
	test_Equal(0,size);
	Trace2(4);
	size = Trace.GetData(data);
	test_Equal(0,size);
	Trace.SetFilter(BTrace::ETest1,1);
	Trace2(4);
	size = Trace.GetData(data);
	test_Equal(0,size);
	Trace1(4);
	size = Trace.GetData(data);
	test(CheckSize(data,size,4));
	BaseSize = size - 4;
	test.Printf(_L("BaseSize=%d\n"), BaseSize);

	test.Next(_L("Test Empty()"));
	Trace.Empty();
	size = Trace.GetData(data);
	test_Equal(0,size);

	test.Next(_L("Test DataUsed()"));
	Trace1(0);
	size = Trace.GetData(data);
	test(CheckSize(data,size,0));
	Trace.DataUsed();
	size = Trace.GetData(data);
	test_Equal(0,size);

	test.Next(_L("Test RequestData()"));
	TRequestStatus s1;
	TRequestStatus s2;
	RTimer timer;
	test_KErrNone(timer.CreateLocal());

	// immediate notification...
	Trace.RequestData(s1,0);
	test_Equal(KRequestPending, s1.Int());
	timer.After(s2,5*1000000);
	Trace1(4);
	User::WaitForRequest(s1,s2);
	test_KErrNone(s1.Int());
	timer.Cancel();
	User::WaitForRequest(s2);

	// immediate notification with size>n ...
	Trace.RequestData(s1,BaseSize+8);
	test_Equal(KRequestPending,s1.Int());
	timer.After(s2,5*1000000);
	Trace1(20);
	User::WaitForRequest(s1,s2);
	test_KErrNone(s1.Int());
	timer.Cancel();
	User::WaitForRequest(s2);

	size = Trace.GetData(data);
	test_Compare(size, >= , BaseSize+8);
	Trace.DataUsed();
	size = Trace.GetData(data);
	test_Equal(0,size);

	// delayed notification...
	Trace.RequestData(s1,0);
	timer.After(s2,5*1000000);
	Trace1(4,500000);
	test_Equal(KRequestPending,s1.Int());
	User::WaitForRequest(s1,s2);
	test_KErrNone(s1.Int());
	timer.Cancel();
	User::WaitForRequest(s2);
	size = Trace.GetData(data);
	test(CheckSize(data,size,4));
	Trace.DataUsed();

	// delayed notification with size>n...
	Trace.RequestData(s1,BaseSize+8);
	Trace1(4,500000);
	test_Equal(KRequestPending,s1.Int());
	timer.After(s2,1000000);
	User::WaitForRequest(s1,s2);
	test_KErrNone(s2.Int());
	timer.After(s2,5*1000000);
	Trace1(20,500000);
	test_Equal(KRequestPending,s1.Int());
	User::WaitForRequest(s1,s2);
	test_KErrNone(s1.Int());
	timer.Cancel();
	User::WaitForRequest(s2);

	size = Trace.GetData(data);
	test_Compare(size, >=, BaseSize+8);
	Trace.DataUsed();
	size = Trace.GetData(data);
	test_Equal(0,size);

	test.Next(_L("Test RequestData() when data is already available"));
	Trace1(4);
	Trace.RequestData(s1,0);
	test_KErrNone(s1.Int());
	User::WaitForRequest(s1);
	size = Trace.GetData(data);
	test(CheckSize(data,size,4));
	Trace.DataUsed();
	size = Trace.GetData(data);
	test_Equal(0,size);

	Trace1(4);
	Trace.RequestData(s1,1);
	test_KErrNone(s1.Int());
	User::WaitForRequest(s1);
	size = Trace.GetData(data);
	test(CheckSize(data,size,4));
	Trace.DataUsed();
	size = Trace.GetData(data);
	test_Equal(0,size);

	test.Next(_L("Test RequestData() for ISR disabled traces"));
	Trace.RequestData(s1,0);
	test_Equal(KRequestPending,s1.Int());
	timer.After(s2,5*1000000);
	TraceTest.Trace(RBTraceTest::EContextIntsOff,KTestTrace1,4);
	User::WaitForRequest(s1,s2);
	test_KErrNone(s1.Int());
	timer.Cancel();
	User::WaitForRequest(s2);
	size = Trace.GetData(data);
	test(CheckSize(data,size,4));
	Trace.DataUsed();
	size = Trace.GetData(data);
	test_Equal(0,size);

	test.Next(_L("Test CancelRequestData()"));
	Trace.RequestData(s1,0);
	test_Equal(KRequestPending,s1.Int());
	Trace.CancelRequestData();
	User::WaitForRequest(s1);
	test_Equal(KErrCancel,s1.Int());

	test.Next(_L("Test trace data contents"));
	Trace1(0);
	size = Trace.GetData(data);
	test(CheckSize(data,size,0));
 	test(CheckTrace1(data,size));
	if(data[BTrace::EFlagsIndex]&BTrace::ETimestampPresent)
		test.Printf(_L("Timestamps are present\n"));
	else
		test.Printf(_L("Timestamps are NOT present\n"));
	Trace.DataUsed();

	Trace1(4);
	size = Trace.GetData(data);
	test(CheckSize(data,size,4));
	test(CheckTrace1(data,size));
	Trace.DataUsed();

	TInt i;
	for(i=0; i<=8+(TInt)KMaxBTraceDataArray; i++)
		{
		Trace1(i);
		size = Trace.GetData(data);
		test(CheckSize(data,size,i));
		test(CheckTrace1(data,size));
		Trace.DataUsed();
		}
	Trace1(i);
	size = Trace.GetData(data);
	test(data[BTrace::EFlagsIndex]&BTrace::ERecordTruncated);
	test(CheckSize(data,size,i-1));
	test(CheckTrace1(data,size));
	Trace.DataUsed();

	test.Next(_L("Test BufferSize() and ResizeBuffer()"));
	TInt oldSize = Trace.BufferSize();
	Trace1(50);
	size = Trace.GetData(data);
	test(CheckSize(data,size,50));
	r = Trace.ResizeBuffer(oldSize+0x1000);
	test_KErrNone(r);
	size = Trace.BufferSize();
	test_Equal(oldSize+0x1000,size);
	Trace.SetMode(aMode);
	size = Trace.GetData(data);
	test_Equal(0,size);
	Trace1(40);
	size = Trace.GetData(data);
	test(CheckSize(data,size,40));
	Trace.DataUsed();
	r = Trace.ResizeBuffer(oldSize);
	test_KErrNone(r);
	size = Trace.BufferSize();
	test_Equal(oldSize,size);

	test.End();
	}




//---------------------------------------------
//! @SYMTestCaseID KBASE-T_BTRACE-0060
//! @SYMTestType UT
//! @SYMPREQ PREQ1030
//! @SYMTestCaseDesc Test traces generated from user code.
//! @SYMTestActions Generate traces using BTrace0, BTrace4, BTrace8, BTrace12,
//!		and BTraceN macros.
//! @SYMTestExpectedResults All trace contents captured by RBTrace match those specified
//!		at point of trace generation.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void TestUserTrace(TUint aMode)
	{
	aMode |= RBTrace::EEnable;

	TUint8* data;
	TInt size;

	test.Start(_L("Reset trace buffer"));
	Trace.SetMode(0);
	Trace.Empty();
	Trace.SetMode(RBTrace::EEnable);
	Trace.SetFilter(BTrace::ETest1,1);
	Trace.SetFilter(BTrace::ETest2,0);

	test.Next(_L("BTrace0"));
	BTrace0(BTrace::ETest1,KTest1SubCategory);
	size = Trace.GetData(data);
	test(CheckSize(data,size,0));
	test(CheckTrace1(data,size));
	Trace.DataUsed();

	test.Next(_L("BTrace4"));
	BTrace4(BTrace::ETest1,KTest1SubCategory,*(TUint32*)(KTestTrace1+4));
	size = Trace.GetData(data);
	test(CheckSize(data,size,4));
	test(CheckTrace1(data,size));
	Trace.DataUsed();

	test.Next(_L("BTrace8"));
	BTrace8(BTrace::ETest1,KTest1SubCategory,*(TUint32*)(KTestTrace1+4),*(TUint32*)(KTestTrace1+8));
	size = Trace.GetData(data);
	test(CheckSize(data,size,8));
	test(CheckTrace1(data,size));
	Trace.DataUsed();

	test.Next(_L("BTrace12"));
	BTrace12(BTrace::ETest1,KTest1SubCategory,*(TUint32*)(KTestTrace1+4),*(TUint32*)(KTestTrace1+8),*(TUint32*)(KTestTrace1+12));
	size = Trace.GetData(data);
	test(CheckSize(data,size,12));
	test(CheckTrace1(data,size));
	Trace.DataUsed();

	test.Next(_L("BTraceN"));
	TInt i;
	for(i=8; i<=8+(TInt)KMaxBTraceDataArray; i++)
		{
		BTraceN(BTrace::ETest1,KTest1SubCategory,*(TUint32*)(KTestTrace1+4),*(TUint32*)(KTestTrace1+8),KTestTrace1+12,i-8);
		size = Trace.GetData(data);
		test(CheckSize(data,size,i));
		test(CheckTrace1(data,size));
		Trace.DataUsed();
		}
	BTraceN(BTrace::ETest1,KTest1SubCategory,*(TUint32*)(KTestTrace1+4),*(TUint32*)(KTestTrace1+8),KTestTrace1+12,i-8);
	size = Trace.GetData(data);
	test(data[BTrace::EFlagsIndex]&BTrace::ERecordTruncated);
	test(CheckSize(data,size,i-1));
	test(CheckTrace1(data,size));
	Trace.DataUsed();

	test.End();
	}


TBool CompareFilter2(const TUint32* aUids, TInt aNumUids, TInt aGlobalFilter)
	{
	TUint32* filter2Buffer = (TUint32*)-1; // initialise to invalid value
	TInt filter2Global = 0x80000000; // initialise to invalid value
	TInt filter2Size = Trace.Filter2(filter2Buffer,filter2Global);

	TBool pass = ETrue;
	if(filter2Size!=aNumUids)
		pass = EFalse;
	else if(filter2Global!=aGlobalFilter)
		pass = EFalse;
	else if(0!=Mem::Compare((TUint8*)filter2Buffer,filter2Size,(TUint8*)aUids,aNumUids))
		pass = EFalse;

	delete filter2Buffer;
	return pass;
	}


void TestFilter2()
	{
	test.Start(_L("Get filter2"));
	TUint32* filter2Buffer = (TUint32*)-1; // initialise to invalid value
	TInt filter2Global = 0x80000000; // initialise to invalid value
	TInt filter2Size = Trace.Filter2(filter2Buffer,filter2Global);
	test_NotNegative(filter2Size);
	test_Compare(filter2Buffer, != , (TUint32*)-1);
	test_Compare(filter2Global, != , -1);
	test(CompareFilter2(filter2Buffer,filter2Size,filter2Global));
	Trace.SetFilter(BTrace::ETest1,1);
	TInt r;

	test.Next(_L("Clear filter2"));
	r = Trace.SetFilter2((TUint32*)0,0);
	test_KErrNone(r);
	Trace.SetFilter2(0);
	test(CompareFilter2(0,0,0));

#ifdef _DEBUG
	test.Next(_L("Check SetFilter2's 'New' fails gracefully"));
	__KHEAP_FAILNEXT(1);

	r = Trace.SetFilter2(KBTraceFilterTestUid1,1);
	test_Equal(KErrNoMemory, r);

	__KHEAP_RESET;
#endif

	test.Next(_L("Test set and clear single uid"));
	r = Trace.SetFilter2(KBTraceFilterTestUid1,0);
	test_KErrNone(r);
	test(CompareFilter2(0,0,0));
	r = Trace.SetFilter2(KBTraceFilterTestUid1,1);
	test_KErrNone(r);
	test(CompareFilter2(&KBTraceFilterTestUid1,1,-1));
	r = Trace.SetFilter2(KBTraceFilterTestUid1,0);
	test_Equal(1,r);
	test(CompareFilter2(0,0,0));

	test.Next(_L("Test set multiple uid API"));
	r = Trace.SetFilter2(&KBTraceFilterTestUid1,1);
	test_KErrNone(r);
	test(CompareFilter2(&KBTraceFilterTestUid1,1,-1));
	r = Trace.SetFilter2(KBTraceFilterTestUid1,0);
	test_Equal(1,r);
	test(CompareFilter2(0,0,0));

	test.Next(_L("Test set and clear uids with lots of permutations"));
	TInt itterations = 0;
	const TInt maxUids = 5;
	TInt permute[maxUids] = {0};
	TInt numUids;
	RArray<TUint32> sortedArray(maxUids);
	RArray<TUint32> array(maxUids);
	for(numUids=1; numUids<=maxUids; ++numUids)
		{
		TInt p=0;
		do
			{
			++itterations;
			if(itterations==-1)
				__BREAKPOINT(); // debuging breakpoint for a specific itteration

			// make arrays of uids
			sortedArray.Reset();
			array.Reset();
			TInt i;
			for(i=0; i<numUids; ++i)
				{
				sortedArray.InsertInUnsignedKeyOrder(KBTraceFilterTestUid+permute[i]);
				array.Append(KBTraceFilterTestUid+permute[i]);
				}

			// set filter using single uid api...
			Trace.SetFilter2(0);
			for(i=0; i<numUids; ++i)
				{
				r = Trace.SetFilter2(KBTraceFilterTestUid+permute[i],1);
				test_NotNegative(r);
				}
			test(CompareFilter2(&sortedArray[0],sortedArray.Count(),-1));

			// set filter using multiple uid api...
			Trace.SetFilter2(0);
			r = Trace.SetFilter2(&array[0],array.Count());
			test_NotNegative(r);
			test(CompareFilter2(&sortedArray[0],sortedArray.Count(),-1));

			// remove uids...
			for(i=0; i<numUids; ++i)
				{
				TUint32 removedUid = KBTraceFilterTestUid+permute[i];
				TBool removed = EFalse;
				r = sortedArray.FindInUnsignedKeyOrder(removedUid);
				if(r>=0)
					{
					test(BTrace::CheckFilter2(BTrace::ETest1,removedUid));
					sortedArray.Remove(r);
					removed = ETrue;
					}
				r = Trace.SetFilter2(removedUid,0);
				test_NotNegative(r);
				if(removed)
					{
					test(!BTrace::CheckFilter2(BTrace::ETest1,removedUid));
					}
				r = sortedArray.Count();
				if(r)
					test(CompareFilter2(&sortedArray[0],r,-1));
				else
					{
					test(CompareFilter2(0,0,0));
					break;
					}
				}

			// make next permutation
			p=0;
			while(p<numUids && ++permute[p] == numUids)
				permute[p++] = 0;
			}
		while(p<numUids);
		}

	test.Next(_L("Test global filter"));
	Trace.SetFilter2(0);
	test(CompareFilter2(0,0,0));
	Trace.SetFilter2(1);
	test(CompareFilter2(0,0,1));
	r = Trace.SetFilter2(KBTraceFilterTestUid1,1);
	test_Equal(1,r);
	test(CompareFilter2(0,0,1));
	r = Trace.SetFilter2(KBTraceFilterTestUid1,0);
	test_Equal(KErrNotSupported,r);
	test(CompareFilter2(0,0,1));
	r = Trace.SetFilter2(&KBTraceFilterTestUid1,1);
	test_KErrNone(r);
	test(CompareFilter2(&KBTraceFilterTestUid1,1,-1));

	test.Next(_L("Restore filter2"));
	r = Trace.SetFilter2(filter2Buffer,filter2Size);
	test_KErrNone(r);
	Trace.SetFilter2(filter2Global);
	Trace.SetFilter(BTrace::ETest1,0);
	delete filter2Buffer;

	test.End();
	}

TUint32 ThisTraceContextId;

void TestTrace1(TUint aType,TInt aSize)
	{
	if(!(aType&RBTraceTest::EUserTrace))
		{
		// use driver to create a kernel trace...
		TraceTest.Trace(aType,KTestTrace1,aSize);
		return;
		}

	TInt size = aSize;
	TUint32* data = (TUint32*)KTestTrace1;
	BTrace::TCategory category = BTrace::ETest1;
	TUint subCategory = KTest1SubCategory;
	TUint type = aType&0xff;
	TBool bigTrace = aType&RBTraceTest::EBigTrace;
	TBool filter2Trace = aType&RBTraceTest::EFilter2Trace;

	if(!filter2Trace)
		{
		if(type==BTrace::EPcPresent)
			{
			if(bigTrace)
				{
				BTracePcBig(category,subCategory,data[1],data+2,size-4);
				BTracePcBig(category,subCategory,data[1],data+2,size-4);
				}
			else if(size==0)
				{
				BTracePc0(category,subCategory);
				BTracePc0(category,subCategory);
				}
			else if(size<=4)
				{
				BTracePc4(category,subCategory,data[1]);
				BTracePc4(category,subCategory,data[1]);
				}
			else if(size<=8)
				{
				BTracePc8(category,subCategory,data[1],data[2]);
				BTracePc8(category,subCategory,data[1],data[2]);
				}
			else
				{
				BTracePcN(category,subCategory,data[1],data[2],data+3,size-8);
				BTracePcN(category,subCategory,data[1],data[2],data+3,size-8);
				}
			}
		else if(type==BTrace::EContextIdPresent)
			{
			if(bigTrace)
				{
				BTraceContextBig(category,subCategory,data[1],data+2,size-4);
				BTraceContextBig(category,subCategory,data[1],data+2,size-4);
				}
			else if(size==0)
				{
				BTraceContext0(category,subCategory);
				BTraceContext0(category,subCategory);
				}
			else if(size<=4)
				{
				BTraceContext4(category,subCategory,data[1]);
				BTraceContext4(category,subCategory,data[1]);
				}
			else if(size<=8)
				{
				BTraceContext8(category,subCategory,data[1],data[2]);
				BTraceContext8(category,subCategory,data[1],data[2]);
				}
			else
				{
				BTraceContextN(category,subCategory,data[1],data[2],data+3,size-8);
				BTraceContextN(category,subCategory,data[1],data[2],data+3,size-8);
				}
			}
		else if(type==BTrace::EContextIdPresent+BTrace::EPcPresent)
			{
			if(bigTrace)
				{
				BTraceContextPcBig(category,subCategory,data[1],data+2,size-4);
				BTraceContextPcBig(category,subCategory,data[1],data+2,size-4);
				}
			else if(size==0)
				{
				BTraceContextPc0(category,subCategory);
				BTraceContextPc0(category,subCategory);
				}
			else if(size<=4)
				{
				BTraceContextPc4(category,subCategory,data[1]);
				BTraceContextPc4(category,subCategory,data[1]);
				}
			else if(size<=8)
				{
				BTraceContextPc8(category,subCategory,data[1],data[2]);
				BTraceContextPc8(category,subCategory,data[1],data[2]);
				}
			else
				{
				BTraceContextPcN(category,subCategory,data[1],data[2],data+3,size-8);
				BTraceContextPcN(category,subCategory,data[1],data[2],data+3,size-8);
				}
			}
		else
			{
			if(bigTrace)
				BTraceBig(category,subCategory,data[1],data+2,size-4);
			else if(size==0)
				BTrace0(category,subCategory);
			else if(size<=4)
				BTrace4(category,subCategory,data[1]);
			else if(size<8)
				BTrace8(category,subCategory,data[1],data[2]);
			else
				BTraceN(category,subCategory,data[1],data[2],data+3,size-8);
			}
		}
	else
		{
		if(type==BTrace::EPcPresent)
			{
			if(bigTrace)
				{
				BTraceFilteredPcBig(category,subCategory,data[1],data+2,size-4);
				BTraceFilteredPcBig(category,subCategory,data[1],data+2,size-4);
				}
			else if(size<4)
				{
				// invalid
				}
			else if(size==4)
				{
				BTraceFilteredPc4(category,subCategory,data[1]);
				BTraceFilteredPc4(category,subCategory,data[1]);
				}
			else if(size<=8)
				{
				BTraceFilteredPc8(category,subCategory,data[1],data[2]);
				BTraceFilteredPc8(category,subCategory,data[1],data[2]);
				}
			else
				{
				BTraceFilteredPcN(category,subCategory,data[1],data[2],data+3,size-8);
				BTraceFilteredPcN(category,subCategory,data[1],data[2],data+3,size-8);
				}
			}
		else if(type==BTrace::EContextIdPresent)
			{
			if(bigTrace)
				{
				BTraceFilteredContextBig(category,subCategory,data[1],data+2,size-4);
				BTraceFilteredContextBig(category,subCategory,data[1],data+2,size-4);
				}
			else if(size<4)
				{
				// invalid
				}
			else if(size==4)
				{
				BTraceFilteredContext4(category,subCategory,data[1]);
				BTraceFilteredContext4(category,subCategory,data[1]);
				}
			else if(size<=8)
				{
				BTraceFilteredContext8(category,subCategory,data[1],data[2]);
				BTraceFilteredContext8(category,subCategory,data[1],data[2]);
				}
			else
				{
				BTraceFilteredContextN(category,subCategory,data[1],data[2],data+3,size-8);
				BTraceFilteredContextN(category,subCategory,data[1],data[2],data+3,size-8);
				}
			}
		else if(type==BTrace::EContextIdPresent+BTrace::EPcPresent)
			{
			if(bigTrace)
				{
				BTraceFilteredContextPcBig(category,subCategory,data[1],data+2,size-4);
				BTraceFilteredContextPcBig(category,subCategory,data[1],data+2,size-4);
				}
			else if(size<4)
				{
				// invalid
				}
			else if(size==4)
				{
				BTraceFilteredContextPc4(category,subCategory,data[1]);
				BTraceFilteredContextPc4(category,subCategory,data[1]);
				}
			else if(size<=8)
				{
				BTraceFilteredContextPc8(category,subCategory,data[1],data[2]);
				BTraceFilteredContextPc8(category,subCategory,data[1],data[2]);
				}
			else
				{
				BTraceFilteredContextPcN(category,subCategory,data[1],data[2],data+3,size-8);
				BTraceFilteredContextPcN(category,subCategory,data[1],data[2],data+3,size-8);
				}
			}
		else
			{
			if(bigTrace)
				BTraceFilteredBig(category,subCategory,data[1],data+2,size-4);
			else if(size<4)
				{
				// invalid
				}
			else if(size==4)
				BTraceFiltered4(category,subCategory,data[1]);
			else if(size<8)
				BTraceFiltered8(category,subCategory,data[1],data[2]);
			else
				BTraceFilteredN(category,subCategory,data[1],data[2],data+3,size-8);
			}
		}
	}



//---------------------------------------------
//! @SYMTestCaseID KBASE-T_BTRACE-0062-0063
//! @SYMTestType UT
//! @SYMPREQ PREQ1030
//! @SYMTestCaseDesc Test traces which specify thread context and/or program counter values.
//! @SYMTestActions Generate traces from user and kernel code using the BTracePcX,
//!		BTraceContextX and BTraceContextPcX macros. Kernel traces are additionaly
//!		generated in ISR and IDFC context.
//! @SYMTestExpectedResults All trace contents captured by RBTrace match those specified
//!		at point of trace generation. Also, where appropriate, PC and/or Context ID values
//!		are present and correct.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void TestTrace(TBool aUserTrace, TBool aFilter2)
	{
	test.Start(_L("Reset trace buffer"));
	TInt oldSize = Trace.BufferSize();
	TInt r = Trace.ResizeBuffer(0x100000);
	test_KErrNone(r);
	Trace.SetMode(RBTrace::EEnable);

	// dummy trace do get current thread context id
	Trace.SetFilter(BTrace::ETest1,0);
	ThisTraceContextId = TraceTest.Trace(BTrace::EContextIdPresent,KTestTrace1,0);

	// create a test filter...
	TInt extraFlags = 0;
	if(aUserTrace)
		extraFlags |= RBTraceTest::EUserTrace;
	TInt minSize = 0;
	if(aFilter2)
		{
		extraFlags |= RBTraceTest::EFilter2Trace;
		minSize += 4;
		}

	TInt filterMode;
	for(filterMode=0; filterMode<(aFilter2?6:2); ++filterMode)
		{

		// setup filters...
		Trace.SetFilter(BTrace::ETest1,1);
		Trace.SetFilter2(BigFilter2,KNumBTraceFilterTestUids);
		if(filterMode==0 || filterMode==2)
			Trace.SetFilter(BTrace::ETest1,0); // disable in primary filter
		if(filterMode==0 || filterMode==1)
			Trace.SetFilter2(KBTraceFilterTestUid1,0); // disable in secondary filter
		if(filterMode==4)
			Trace.SetFilter2(0); // disable entire secondary filter
		if(filterMode==5)
			Trace.SetFilter2(1); // enable entire secondary filter

		// expectTrace is true if we expect trace to be output...
		TBool expectTrace = aFilter2 ? (filterMode==3 || filterMode==5) : filterMode&1;

		switch(filterMode)
			{
		case 0: test.Next(_L("Test with primary filter OFF, secondary filter OFF")); break;
		case 1: test.Next(_L("Test with primary filter ON, secondary filter OFF")); break;
		case 2: test.Next(_L("Test with primary filter OFF, secondary filter ON")); break;
		case 3: test.Next(_L("Test with primary filter ON, secondary filter ON")); break;
		case 4: test.Next(_L("Test with primary filter ON, global secondary filter OFF")); break;
		case 5: test.Next(_L("Test with primary filter ON, global secondary filter ON")); break;
			}

		test.Start(_L("Traces without special context"));
		TInt i;
		for(i=minSize; i<=8+(TInt)KMaxBTraceDataArray; i++)
			{
			TestTrace1(extraFlags,i);

			TUint8* data;
			TInt size;
			size = Trace.GetData(data);
			if(!expectTrace)
				{
				test(!size);
				continue;
				}

			test(CheckSize(data,size,i));
			test(CheckTrace1(data,size,KTest1SubCategory));

			test(!(data[BTrace::EFlagsIndex]&BTrace::EContextIdPresent));
			test(!(data[BTrace::EFlagsIndex]&BTrace::EPcPresent));

			Trace.DataUsed();
			}

		test.Next(_L("Traces with PC"));
		for(i=minSize; i<=8+(TInt)KMaxBTraceDataArray; i++)
			{
			TestTrace1(BTrace::EPcPresent|extraFlags,i);

			TUint8* data;
			TUint8* data2;
			TInt size;
			size=Trace.GetData(data);
			if(!expectTrace)
				{
				test(!size);
				continue;
				}
			size /= 2;
			data2 = data+size;

			test(CheckSize(data,size,i));
			test(CheckTrace1(data,size,KTest1SubCategory));

			test(CheckSize(data2,size,i));
			test(CheckTrace1(data2,size,KTest1SubCategory));

			test(!(data[BTrace::EFlagsIndex]&BTrace::EContextIdPresent));
			test((data[BTrace::EFlagsIndex]&BTrace::EPcPresent));

			TInt offset = ContextOffset(data);
			test_Compare( ((TUint32*)(data+offset))[0], != ,((TUint32*)(data2+offset))[0]);

			Trace.DataUsed();
			}

		test.Next(_L("Traces with Context ID"));
		for(i=minSize; i<=8+(TInt)KMaxBTraceDataArray; i++)
			{
			TestTrace1(BTrace::EContextIdPresent|extraFlags,i);

			TUint8* data;
			TUint8* data2;
			TInt size;
			size=Trace.GetData(data);
			if(!expectTrace)
				{
				test(!size);
				continue;
				}
			size /= 2;
			data2 = data+size;

			test(CheckSize(data,size,i));
			test(CheckTrace1(data,size,KTest1SubCategory));

			test(CheckSize(data2,size,i));
			test(CheckTrace1(data2,size,KTest1SubCategory));

			test((data[BTrace::EFlagsIndex]&BTrace::EContextIdPresent));
			test(!(data[BTrace::EFlagsIndex]&BTrace::EPcPresent));

			TUint offset = ContextOffset(data);
			test_Equal(ThisTraceContextId, ((TUint32*)(data+offset))[0]);
			test_Equal(ThisTraceContextId, ((TUint32*)(data2+offset))[0]);
			Trace.DataUsed();
			}

		test.Next(_L("Traces with Context ID and PC"));
		for(i=minSize; i<=8+(TInt)KMaxBTraceDataArray; i++)
			{
			TestTrace1(BTrace::EContextIdPresent|BTrace::EPcPresent|extraFlags,i);

			TUint8* data;
			TUint8* data2;
			TInt size;
			size=Trace.GetData(data);
			if(!expectTrace)
				{
				test(!size);
				continue;
				}
			size /= 2;
			data2 = data+size;

			test(CheckSize(data,size,i));
			test(CheckTrace1(data,size,KTest1SubCategory));

			test(CheckSize(data2,size,i));
			test(CheckTrace1(data2,size,KTest1SubCategory));

			test((data[BTrace::EFlagsIndex]&BTrace::EContextIdPresent));
			test((data[BTrace::EFlagsIndex]&BTrace::EPcPresent));

			TUint offset = ContextOffset(data);

			test_Equal(ThisTraceContextId, ((TUint32*)(data+offset))[0]);
			test_Equal(ThisTraceContextId, ((TUint32*)(data2+offset))[0]);
			test_Compare( ((TUint32*)(data+offset))[1], != ,((TUint32*)(data2+offset))[1]);

			Trace.DataUsed();
			}

		if(!aUserTrace)
			{
			test.Next(_L("Traces with Context ID in ISR mode"));
			for(i=minSize; i<=8+(TInt)KMaxBTraceDataArray; i++)
				{
				TraceTest.Trace(BTrace::EContextIdPresent|RBTraceTest::EContextIsr|extraFlags,KTestTrace1,i);

				TUint8* data;
				TUint8* data2;
				TInt size;
				size=Trace.GetData(data);
				if(!expectTrace)
					{
					test(!size);
					continue;
					}
				size /= 2;
				data2 = data+size;

				test(CheckSize(data,size,i));
				test(CheckTrace1(data,size,KTest1SubCategory));

				test(CheckSize(data2,size,i));
				test(CheckTrace1(data2,size,KTest1SubCategory));

				test((data[BTrace::EFlagsIndex]&BTrace::EContextIdPresent));
				test(!(data[BTrace::EFlagsIndex]&BTrace::EPcPresent));

				TUint offset = ContextOffset(data);
				test( ((TUint32*)(data+offset))[0] == 2 );
				test( ((TUint32*)(data2+offset))[0] == 2 );

				Trace.DataUsed();
				}

			test.Next(_L("Traces with Context ID in IDFC mode"));
			for(i=minSize; i<=8+(TInt)KMaxBTraceDataArray; i++)
				{
				TraceTest.Trace(BTrace::EContextIdPresent|RBTraceTest::EContextIDFC|extraFlags,KTestTrace1,i);

				TUint8* data;
				TUint8* data2;
				TInt size;
				size=Trace.GetData(data);
				if(!expectTrace)
					{
					test(!size);
					continue;
					}
				size /= 2;
				data2 = data+size;

				test(CheckSize(data,size,i));
				test(CheckTrace1(data,size,KTest1SubCategory));

				test(CheckSize(data2,size,i));
				test(CheckTrace1(data2,size,KTest1SubCategory));

				test((data[BTrace::EFlagsIndex]&BTrace::EContextIdPresent));
				test(!(data[BTrace::EFlagsIndex]&BTrace::EPcPresent));

				TUint offset = ContextOffset(data);
				test( ((TUint32*)(data+offset))[0] == 3 );
				test( ((TUint32*)(data2+offset))[0] == 3 );

				Trace.DataUsed();
				}
			}
		test.End();
		r = Trace.ResizeBuffer(0x100000);	// avoid buffer wrap problems
		test_KErrNone(r);
		Trace.SetMode(RBTrace::EEnable);
		}

	test.Next(_L("Restore buffer"));
	r = Trace.ResizeBuffer(oldSize);
	test_KErrNone(r);
	Trace.SetFilter2(0);

	test.End();
	}


TUint32 BigTraceId = 0;

TBool BigTraceFirst = 0;
TBool BigTraceMiddle = 0;
TBool BigTraceEnd = 0;

void BigTraceBeginTest()
	{
	BigTraceFirst = 0;
	BigTraceMiddle = 0;
	BigTraceEnd = 0;
	}

TBool BigTraceEndTest()
	{
	return BigTraceFirst&&BigTraceMiddle&&BigTraceEnd;
	}

TBool DoCheckBigTrace1(TUint8*& aData, TUint32 aOutSize, TUint32& aOffset, TUint32 aExtraIds = 0)
	{
	TUint32* ptr = (TUint32*)aData;
	if(aData[BTrace::ECategoryIndex]!=BTrace::ETest1)
		return EFalse;
	if(aData[BTrace::ESubCategoryIndex]!=KTest1SubCategory)
		return EFalse;
//	TUint32 header = *ptr++;
	++ptr;

	if(aData[BTrace::EFlagsIndex]&BTrace::ERecordTruncated)
		return EFalse;

	if(!(aData[BTrace::EFlagsIndex]&BTrace::EHeader2Present))
		return EFalse;
	TUint32 header2 = *ptr++;
	if(aData[BTrace::EFlagsIndex]&BTrace::ETimestampPresent)
		++ptr;
	if(aData[BTrace::EFlagsIndex]&BTrace::ETimestamp2Present)
		++ptr;

	if(aData[BTrace::EFlagsIndex]&BTrace::EContextIdPresent)
		++ptr;
	if(aData[BTrace::EFlagsIndex]&BTrace::EPcPresent)
		++ptr;

	if(!(aData[BTrace::EFlagsIndex]&BTrace::EExtraPresent))
		return EFalse;
	TUint id = *ptr++;

	if(*ptr++ != aOutSize)
		return EFalse;

	if(aOffset && *ptr++ != aOffset)
		return EFalse;

	TInt size = aData[BTrace::ESizeIndex]-((TInt)ptr-(TInt)aData);
	TUint8* data = (TUint8*)ptr;
	TUint8* out = (TUint8*)KTestTrace1+4+aOffset;

	if(!aOffset)
		{
		if((header2&BTrace::EMultipartFlagMask) != BTrace::EMultipartFirst)
			return EFalse;
		BigTraceId = id;
		aOffset += size-4-aExtraIds;
		BigTraceFirst = ETrue;
		}
	else
		{
		if(id!=BigTraceId)
			return EFalse;
		aOffset += size;
		out += 4 + aExtraIds;
		if(aOffset==aOutSize)
			{
			if((header2&BTrace::EMultipartFlagMask) != BTrace::EMultipartLast)
				return EFalse;
			BigTraceEnd = ETrue;
			}
		else
			{
			if((header2&BTrace::EMultipartFlagMask) != BTrace::EMultipartMiddle)
				return EFalse;
			BigTraceMiddle = ETrue;
			}
		}
	if(aOffset>aOutSize)
		return EFalse;

	while(--size>=0)
		if(*data++!=*out++)
			return EFalse;
	aData = data;

	return ETrue;
	}



TBool CheckBigTrace1(TUint8* aData, TInt aSize, TInt aOutSize, TInt aSubCategory=KTest1SubCategory, TUint32 aExtraIds = 0)
	{
	if(aOutSize<=(TInt)KMaxBTraceDataArray+8)
		{
		if(!CheckSize(aData,aSize,aOutSize))
			return EFalse;
		if(!CheckTrace1(aData,aSize,aSubCategory))
			return EFalse;
		}
	else
		{
		TUint8* end = aData+aSize;
		aOutSize -= 4 + aExtraIds; // first 4 bytes of trace are always present, and don't count towards 'size' of multipart trace
		TUint32 aOffset = 0;
		while(aOffset<TUint32(aOutSize))
			if(!DoCheckBigTrace1(aData,aOutSize,aOffset,aExtraIds))
				return EFalse;
		aData = (TUint8*)(((TInt)aData+3)&~3);
		if(aData != end)
			return EFalse;
		}
	return ETrue;
	}


//---------------------------------------------
//! @SYMTestCaseID KBASE-T_BTRACE-0061
//! @SYMTestType UT
//! @SYMPREQ PREQ1030
//! @SYMTestCaseDesc Test Big (mutipart) kernel traces.
//! @SYMTestActions Generate traces from kernel code using the BTraceBig,
//!		BTracePcBig, BTraceContextBig and BTraceContextPcBig macros.
//! @SYMTestExpectedResults Traces where broken down into mutiple parts and
//!		all trace contents captured by RBTrace matched those specified
//!		at point of trace generation. Also, where appropriate, PC and/or
//!		Context ID values are present and correct.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void TestBig(TBool aUserTrace, TBool aFilter2)
	{
	test.Start(_L("Reset trace buffer"));
	TInt oldSize = Trace.BufferSize();
	TInt r = Trace.ResizeBuffer(0x100000);
	test_KErrNone(r);
	Trace.SetMode(RBTrace::EEnable);

	// dummy trace do get current thread context id
	Trace.SetFilter(BTrace::ETest1,0);
	ThisTraceContextId = TraceTest.Trace(BTrace::EContextIdPresent,KTestTrace1,0);

	// create a test filter...
	TInt extraFlags = 0;
	if(aUserTrace)
		extraFlags |= RBTraceTest::EUserTrace;
	TInt minSize = 4;
	if(aFilter2)
		{
		extraFlags |= RBTraceTest::EFilter2Trace;
		minSize += 4;
		}

	TInt filterMode;
	for(filterMode=0; filterMode<(aFilter2?6:2); ++filterMode)
		{

		// setup filters...
		Trace.SetFilter(BTrace::ETest1,1);
		Trace.SetFilter2(BigFilter2,KNumBTraceFilterTestUids);
		if(filterMode==0 || filterMode==2)
			Trace.SetFilter(BTrace::ETest1,0); // disable in primary filter
		if(filterMode==0 || filterMode==1)
			Trace.SetFilter2(KBTraceFilterTestUid1,0); // disable in secondary filter
		if(filterMode==4)
			Trace.SetFilter2(0); // disable entire secondary filter
		if(filterMode==5)
			Trace.SetFilter2(1); // enable entire secondary filter

		// expectTrace is true if we expect trace to be output...
		TBool expectTrace = aFilter2 ? (filterMode==3 || filterMode==5) : filterMode&1;

		switch(filterMode)
			{
		case 0: test.Next(_L("Test with primary filter OFF, secondary filter OFF")); break;
		case 1: test.Next(_L("Test with primary filter ON, secondary filter OFF")); break;
		case 2: test.Next(_L("Test with primary filter OFF, secondary filter ON")); break;
		case 3: test.Next(_L("Test with primary filter ON, secondary filter ON")); break;
		case 4: test.Next(_L("Test with primary filter ON, global secondary filter OFF")); break;
		case 5: test.Next(_L("Test with primary filter ON, global secondary filter ON")); break;
			}

		test.Start(_L("Big traces without special context"));
		TInt i;
		for(i=minSize; i<=(TInt)sizeof(KTestTrace1)-4; i++)
			{
			TestTrace1(RBTraceTest::EBigTrace|extraFlags,i);

			TUint8* data;
			TInt size;
			size = Trace.GetData(data);
			if(!expectTrace)
				{
				test(!size);
				continue;
				}
			test(CheckBigTrace1(data,size,i,KTest1SubCategory));
			Trace.DataUsed();
			}

		test.Next(_L("Big traces with PC"));
		BigTraceBeginTest();
		for(i=minSize; i<=(TInt)sizeof(KTestTrace1)-4; i++)
			{
			TraceTest.Trace(RBTraceTest::EBigTrace|BTrace::EPcPresent|extraFlags,KTestTrace1,i);

			TUint8* data;
			TUint8* data2;
			TInt size;
			size=Trace.GetData(data);
			if(!expectTrace)
				{
				test(!size);
				continue;
				}
			size /= 2;
			data2 = data+size;

			test(CheckBigTrace1(data,size,i,KTest1SubCategory));
			test(CheckBigTrace1(data2,size,i,KTest1SubCategory));

			test(!(data[BTrace::EFlagsIndex]&BTrace::EContextIdPresent));
			test((data[BTrace::EFlagsIndex]&BTrace::EPcPresent));

			TInt offset = ContextOffset(data);
			test( ((TUint32*)(data+offset))[0] != ((TUint32*)(data2+offset))[0]);

			Trace.DataUsed();
			}
		test_Equal(expectTrace,BigTraceEndTest()); // check we actually got mutilpart traces

		test.Next(_L("Big traces with Context ID"));
		BigTraceBeginTest();
		for(i=minSize; i<=(TInt)sizeof(KTestTrace1)-4; i++)
			{
			TestTrace1(RBTraceTest::EBigTrace|BTrace::EContextIdPresent|extraFlags,i);

			TUint8* data;
			TUint8* data2;
			TInt size;
			size=Trace.GetData(data);
			if(!expectTrace)
				{
				test(!size);
				continue;
				}
			size /= 2;
			data2 = data+size;

			test(CheckBigTrace1(data,size,i,KTest1SubCategory));
			test(CheckBigTrace1(data2,size,i,KTest1SubCategory));

			test((data[BTrace::EFlagsIndex]&BTrace::EContextIdPresent));
			test(!(data[BTrace::EFlagsIndex]&BTrace::EPcPresent));

			TUint offset = ContextOffset(data);
			test_Equal(ThisTraceContextId, ((TUint32*)(data+offset))[0]);
			test_Equal(ThisTraceContextId, ((TUint32*)(data2+offset))[0]);

			Trace.DataUsed();
			}
		test_Equal(expectTrace,BigTraceEndTest()); // check we actually got mutilpart traces

		test.Next(_L("Big traces with Context ID and PC"));
		BigTraceBeginTest();
		for(i=minSize; i<=(TInt)sizeof(KTestTrace1)-4; i++)
			{
			TestTrace1(RBTraceTest::EBigTrace|BTrace::EContextIdPresent|BTrace::EPcPresent|extraFlags,i);

			TUint8* data;
			TUint8* data2;
			TInt size;
			size=Trace.GetData(data);
			if(!expectTrace)
				{
				test(!size);
				continue;
				}
			size /= 2;
			data2 = data+size;

			test(CheckBigTrace1(data,size,i,KTest1SubCategory));
			test(CheckBigTrace1(data2,size,i,KTest1SubCategory));

			test((data[BTrace::EFlagsIndex]&BTrace::EContextIdPresent));
			test((data[BTrace::EFlagsIndex]&BTrace::EPcPresent));

			TUint offset = ContextOffset(data);
			test_Equal(ThisTraceContextId, ((TUint32*)(data+offset))[0] );
			test_Equal(ThisTraceContextId, ((TUint32*)(data2+offset))[0] );
			test_Compare( ((TUint32*)(data+offset))[1], != ,((TUint32*)(data2+offset))[1]);

			Trace.DataUsed();
			}
		test_Equal(expectTrace,BigTraceEndTest()); // check we actually got mutilpart traces
		test.End();
		}

	test.Next(_L("Restore buffer"));
	r = Trace.ResizeBuffer(oldSize);
	test_KErrNone(r);
	Trace.SetFilter2(0);

	test.End();
	}


TUint MainThreadTraceId = 0;

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_BTRACE-0064
//! @SYMTestType UT
//! @SYMPREQ PREQ1030
//! @SYMTestCaseDesc Test BTrace category EThreadIdentification.
//! @SYMTestActions Enable the EThreadIdentification trace category, causing it to be 'primed'.
//!		Rename a thread and a process, then create and destory a thread.
//!		No actions are performed if the kernel doesn't support this trace category.
//! @SYMTestExpectedResults All test actions produced traces and their contents matched
//!		the expected results.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void TestThreadIdentification()
	{
	if(KErrNotSupported==Trace.Filter(BTrace::EThreadIdentification))
		{
		test.Start(_L("Trace category not supported by this build of kernel."));
		test.End();
		return;
		}

	TBuf8<KMaxKernelName> threadName = RThread().Name().Collapse();
	TUint threadId = RThread().Id();
	TBuf8<KMaxKernelName> processName = RProcess().Name().Collapse();

	TUint32 threadTraceId = 0;
	TUint32 processTraceId = 0;
	TUint8* data;
	TInt size;

	test.Start(_L("Reset trace buffer"));
	Trace.SetMode(0);
	TInt oldSize = Trace.BufferSize();
	TInt r = Trace.ResizeBuffer(0x100000);
	test_KErrNone(r);
	Trace.SetFilter(BTrace::EThreadIdentification,0);
	Trace.SetMode(RBTrace::EEnable);

	test.Next(_L("Test category is primed correct when it is enabled"));
	Trace.SetFilter(BTrace::EThreadIdentification,1);
	// search for current thread in trace...
	TUint8* trace;
	size=Trace.GetData(trace);
	test_NotNull(size);
	TUint8* end = trace+size;
	for(data=trace; data<end; data=BTrace::NextRecord(data))
		if(data[BTrace::ECategoryIndex]==BTrace::EThreadIdentification)
			if(data[BTrace::ESubCategoryIndex]==BTrace::EProcessName)
				if(processName==Text(data))
					{
					processTraceId = Body(data)[1];
					break;
					}
	test_Compare(data, < , end);
	for(; data<end; data=BTrace::NextRecord(data))
		if(data[BTrace::ECategoryIndex]==BTrace::EThreadIdentification)
			if(data[BTrace::ESubCategoryIndex]==BTrace::EThreadName)
				if(processTraceId==Body(data)[1])
					if(threadName==Text(data))
						{
						threadTraceId = Body(data)[0];
						break;
						}
	test_Compare(data, < , end);
	for(; data<end; data=BTrace::NextRecord(data))
		if(data[BTrace::ECategoryIndex]==BTrace::EThreadIdentification)
			if(data[BTrace::ESubCategoryIndex]==BTrace::EThreadId)
				if(threadTraceId==Body(data)[0])
					if(processTraceId==Body(data)[1])
						if(threadId==Body(data)[2])
							{
							break;
							}
	test_Compare(data, < ,end);
	Trace.DataUsed();
	MainThreadTraceId = threadTraceId;

	test.Next(_L("Test thread rename traces"));
	test_KErrNone(User::RenameThread(_L("t_btrace-main")));
	threadName = RThread().Name().Collapse();
	size=Trace.GetData(data);
	test_NotNull(size);
	end = data+size;
	test_Equal(BTrace::EThreadIdentification,data[BTrace::ECategoryIndex]);
	test_Equal(BTrace::EThreadName, data[BTrace::ESubCategoryIndex]);
	test_Equal(threadTraceId, Body(data)[0]);
	test_Equal(processTraceId, Body(data)[1]);
	test(threadName==Text(data));
	data=BTrace::NextRecord(data);
	test(data==end);
	Trace.DataUsed();

	test.Next(_L("Test process rename traces"));
	test_KErrNone(User::RenameProcess(_L("T_BTRACE-renamed")));
	processName = RProcess().Name().Collapse();
	size=Trace.GetData(data);
	test_NotNull(size);
	end = data+size;
	test_Equal(BTrace::EThreadIdentification,data[BTrace::ECategoryIndex]);
	test_Equal(BTrace::EProcessName ,data[BTrace::ESubCategoryIndex]);
	test_Equal(processTraceId, Body(data)[1]);
	test(processName==Text(data));
	data=BTrace::NextRecord(data);
	test(data==end);
	Trace.DataUsed();

	test.Next(_L("Test thread creation traces"));
	RThread thread;
	test_KErrNone(thread.Create(KNullDesC,0,0x1000,&User::Allocator(),0));
	threadName = thread.Name().Collapse();
	threadId = thread.Id();
	size=Trace.GetData(data);
	test_NotNull(size);
	end = data+size;

	test_Equal(BTrace::EThreadIdentification,data[BTrace::ECategoryIndex]);
	test_Equal(BTrace::ENanoThreadCreate ,data[BTrace::ESubCategoryIndex]);
	threadTraceId = Body(data)[0];
	data=BTrace::NextRecord(data);
	test_Compare(data, < ,end);

	test_Equal(BTrace::EThreadIdentification,data[BTrace::ECategoryIndex]);
	test_Equal(BTrace::EProcessName ,data[BTrace::ESubCategoryIndex]);
	test_Equal(processTraceId, Body(data)[1]);
	test(processName==Text(data));
	data=BTrace::NextRecord(data);
	test_Compare(data, < ,end);

	test_Equal(BTrace::EThreadIdentification,data[BTrace::ECategoryIndex]);
	test_Equal(BTrace::EThreadCreate ,data[BTrace::ESubCategoryIndex]);
	test_Equal(threadTraceId, Body(data)[0]);
	processTraceId = Body(data)[1];
	test(threadName==Text(data));
	data=BTrace::NextRecord(data);
	test_Compare(data, < ,end);

	test_Equal(BTrace::EThreadIdentification,data[BTrace::ECategoryIndex]);
	test_Equal(BTrace::EThreadId ,data[BTrace::ESubCategoryIndex]);
	test_Equal(threadTraceId, Body(data)[0]);
	test_Equal(processTraceId, Body(data)[1]);
	test(threadId==Body(data)[2]);
	data=BTrace::NextRecord(data);
	test_Equal(data,end);
	Trace.DataUsed();

	test.Next(_L("Test thread destruction traces"));
	thread.Kill(0);
	User::After(100000);
	size=Trace.GetData(data);
	test_NotNull(size);
	end = data+size;

	test_Equal(BTrace::EThreadIdentification,data[BTrace::ECategoryIndex]);
	test_Equal(BTrace::ENanoThreadDestroy ,data[BTrace::ESubCategoryIndex]);
	test_Equal(threadTraceId, Body(data)[0]);
	data=BTrace::NextRecord(data);
	test_Compare(data, < ,end);

	test_Equal(BTrace::EThreadIdentification,data[BTrace::ECategoryIndex]);
	test_Equal(BTrace::EThreadDestroy ,data[BTrace::ESubCategoryIndex]);
	test_Equal(threadTraceId, Body(data)[0]);
	test_Equal(processTraceId, Body(data)[1]);
	test(threadId==Body(data)[2]);
	data=BTrace::NextRecord(data);
	test_Equal(data,end);
	Trace.DataUsed();

	test.Next(_L("Cleanup after tests"));
	Trace.SetFilter(BTrace::EThreadIdentification,0);
	Trace.SetMode(0);
	r = Trace.ResizeBuffer(oldSize);
	test_KErrNone(r);

	test.End();
	}


//---------------------------------------------
//! @SYMTestCaseID KBASE-T_BTRACE-0065
//! @SYMTestType UT
//! @SYMPREQ PREQ1030
//! @SYMTestCaseDesc Test BTrace category ECpuUsage.
//! @SYMTestActions Enable the ECpuUsage trace category for one second, then obtain captured
//!		traces using RBTrace.
//! @SYMTestExpectedResults Traces are generated for the sub-categories EIrqStart, EIDFCStart,
//!		EIDFCEnd and ENewThreadContext. The later must include the context ID of the main test
//!		program thread.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void TestCpuUsage()
	{
	if(KErrNotSupported==Trace.Filter(BTrace::ECpuUsage))
		{
		test.Start(_L("Trace category not supported by this build of kernel."));
		test.End();
		return;
		}

	test.Start(_L("Reset trace buffer"));
	Trace.SetMode(0);
	TInt oldSize = Trace.BufferSize();
	TInt r = Trace.ResizeBuffer(0x100000);
	test_KErrNone(r);
	Trace.SetMode(RBTrace::EEnable);

	test.Next(_L("Generate traces for a short time..."));
	Trace.SetFilter(BTrace::ECpuUsage,1);
	User::After(1000*1000);
	Trace.SetFilter(BTrace::ECpuUsage,0);

	test.Next(_L("Process traces"));
	TUint8 subCategories[256] = {0};
	TUint8* data;
	TInt size;
	while((size=Trace.GetData(data))!=0)
		{
		TUint8* end = data+size;
		for( ; data<end; data=BTrace::NextRecord(data))
			{
			test_Equal(BTrace::ECpuUsage,data[BTrace::ECategoryIndex]);
			TUint subCategory = data[BTrace::ESubCategoryIndex];
			TUint8* context = data+ContextOffset(data);
			TUint8* recordEnd = BTrace::NextRecord(data);
			if(subCategory==BTrace::ENewThreadContext)
				{
				test_Equal(context+4, recordEnd); // trace record should end after context data
				if(*(TUint32*)context != MainThreadTraceId)
					continue; // not a context switch to this thread
				}
			else if (subCategory==BTrace::EIrqStart || subCategory==BTrace::EFiqStart)
				{
				// may or may not contain vector number
				if (recordEnd != context)
					{
					test_Equal(context+4, recordEnd);
					}
				}
			else
				{
				test_Equal(context, recordEnd); // trace record should have no context or any data after
				}
			subCategories[subCategory] = 1; // set flag to say we've seen this kind of trace
			}
		Trace.DataUsed();
		}

	test.Next(_L("Check for expected traces"));
	// timer interrupts should have generated IRQ traces...
	test(subCategories[BTrace::EIrqStart]);
	 // User::After should have caused timer interrupt to queue a DFC...
	test(subCategories[BTrace::EIDFCStart]);
	test(subCategories[BTrace::EIDFCEnd]);
	// Current thread must have got scheduled again...
	test(subCategories[BTrace::ENewThreadContext]);

	test.Next(_L("Cleanup after tests"));
	Trace.SetFilter(BTrace::ECpuUsage,0);
	Trace.SetMode(0);
	r = Trace.ResizeBuffer(oldSize);
	test_KErrNone(r);

	test.End();
	}


volatile TBool SoakDone = EFalse; // always false, but stops compiler warnings in SoakThreadFunction

RThread SoakThread[4];

TInt SoakThreadFunction(TAny* aData)
	{
	TUint32 random = 0;
	BTrace::TCategory category = (BTrace::TCategory)*(TUint8*)aData;
	TUint subCategory = 0;
	while(!SoakDone)
		{
		random = random*69069+1;
		TUint size = (random>>24u);

		if(size<4)
			BTrace0(category,subCategory);
		else if(size<=8)
			BTrace4(category,subCategory,((TUint32*)aData)[1]);
		else if(size<12)
			BTrace8(category,subCategory,((TUint32*)aData)[1],((TUint32*)aData)[2]);
		else
			BTraceN(category,subCategory,((TUint32*)aData)[1],((TUint32*)aData)[2],&((TUint32*)aData)[3],size-12);
		++subCategory;
		}
	return 0;
	}


TInt SoakThreadFunction2(TAny* aData)
	{
	TUint32 random = 0;
	BTrace::TCategory category = (BTrace::TCategory)*(TUint8*)aData;
	TUint subCategory = 0;
	while(!SoakDone)
		{
		random = random*69069+1;
		TUint size = (random>>24u);

		TInt outOk;
		if(size<=8)
			outOk = BTraceFiltered4(category,subCategory,((TUint32*)aData)[1]);
		else if(size<12)
			outOk = BTraceFiltered8(category,subCategory,((TUint32*)aData)[1],((TUint32*)aData)[2]);
		else
			outOk = BTraceFilteredN(category,subCategory,((TUint32*)aData)[1],((TUint32*)aData)[2],&((TUint32*)aData)[3],size-12);
		if(outOk)
			++subCategory;
		}
	return 0;
	}


TInt SoakControlThreadFunction(TAny*)
	{
	// randomly mess with thread priorities of soak test threads so
	// we get lots of preemption going on
	const TThreadPriority priorities[] =
		{
		EPriorityMuchLess,
		EPriorityLess,
		EPriorityNormal,
		EPriorityMore
		};

	TUint32 random = 0;
	while(!SoakDone)
		{
		User::AfterHighRes(1);
		random = random*69069+1;
		SoakThread[(random>>16)%3].SetPriority(priorities[(random>>14)&3]);
		}
	return 0;
	}


void ChangeSecondaryFilter()
	{
	// mess with secondary filter...
	static TUint32 random = 0;
	random = random*69069+1;
	switch(random>>29)
		{
		case 0:
			Trace.SetFilter2(KBTraceFilterTestUid1,0);
			break;
		case 1:
			Trace.SetFilter2(KBTraceFilterTestUid1,1);
			break;
		case 2:
			Trace.SetFilter2(KBTraceFilterTestUid2,0);
			break;
		case 3:
			Trace.SetFilter2(KBTraceFilterTestUid2,1);
			break;
		case 4:
			Trace.SetFilter2(0);
			break;
		case 5:
			Trace.SetFilter2(1);
			break;
		case 6:
			Trace.SetFilter2(&KBTraceFilterTestUid1,1);
			break;
		case 7:
			Trace.SetFilter2(&KBTraceFilterTestUid2,1);
			break;
		}
	}

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_BTRACE-0066
//! @SYMTestType UT
//! @SYMPREQ PREQ1030
//! @SYMTestCaseDesc Soak tests
//! @SYMTestActions Generate ordered traces of random size using two worker threads and
//!		read the generated trace in the main test program thread. All threads are scheduled
//!		in a random manner.
//! @SYMTestExpectedResults Captured traces must contain the correct data and any
//!		missing traces must be indicated by the EMissingRecord flag in the next trace.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void TestSoak(TUint aMode,TUint aTimeInSeconds)
	{
	aMode |= RBTrace::EEnable;

	test.Start(_L("Reset trace buffer"));
	TInt oldSize = Trace.BufferSize();
	TInt r = Trace.ResizeBuffer(0x10000);
	test_KErrNone(r);
	Trace.SetMode(aMode);
	Trace.SetFilter(BTrace::ETest1,1);
	Trace.SetFilter(BTrace::ETest2,1);
	Trace.SetFilter2(KBTraceFilterTestUid2,1);

	TUint32* buffer = (TUint32*)Trace.DataChunk().Base();
	test.Printf(_L("Buffer=%x %x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"),buffer,
		buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7],buffer[8],buffer[9]);

	test.Next(_L("Random traces..."));
	Trace1Sequence = 0;
	Trace2Sequence = 0;
	RTimer timer;
	test_KErrNone(timer.CreateLocal());
	SoakThread[0].Duplicate(RThread(),EOwnerProcess);
	test_KErrNone(SoakThread[1].Create(KNullDesC,SoakThreadFunction,0x1000,&User::Allocator(),KTestTrace1));
	test_KErrNone(SoakThread[2].Create(KNullDesC,SoakThreadFunction2,0x1000,&User::Allocator(),KTestTrace2));
	test_KErrNone(SoakThread[3].Create(KNullDesC,SoakControlThreadFunction,0x1000,&User::Allocator(),KTestTrace2));
	// The control thread must be resumed first otherwise time slicing perculiarities
	// can cause the soak threads to take all of the CPU time...
	SoakThread[3].SetPriority(EPriorityMuchMore);
	SoakThread[3].Resume();

	TRequestStatus s;
	TTimeIntervalMicroSeconds32 tickPeriod;
	UserHal::TickPeriod(tickPeriod);
	TInt64 ticks = aTimeInSeconds;
	ticks *= 1000000;
	ticks /= tickPeriod.Int();
	timer.AfterTicks(s,(TInt)ticks);

	// resume threads producing trace output...
	SoakThread[1].Resume();
	SoakThread[2].Resume();

	TInt64 totalSize = 0;
	for(;;)
		{
		ChangeSecondaryFilter();
		const TInt KTraceDataBlockSize = 0x1000;
		TUint8* data;
		TInt size;
		while(s==KRequestPending && (size=Trace.GetData(data))!=0)
			{
//			RDebug::Printf("READ: data=%08x size=%08x\n",data,size);
			if(s!=KRequestPending)
				break;
			CheckTraceData(data,size);
			totalSize += size;
			Trace.DataUsed();
			}
		if(s!=KRequestPending)
			break;
		TRequestStatus waitStatus;
		Trace.RequestData(waitStatus,KTraceDataBlockSize);
		User::WaitForRequest(waitStatus);
		test_KErrNone(waitStatus.Int());
		}
	User::WaitForRequest(s);
	test_Equal(EExitPending, SoakThread[1].ExitType() );
	test_Equal(EExitPending, SoakThread[2].ExitType() );
	test_Equal(EExitPending, SoakThread[3].ExitType() );
	SoakThread[1].Kill(0);
	SoakThread[2].Kill(0);
	SoakThread[3].Kill(0);
	SoakThread[1].Close();
	SoakThread[2].Close();
	SoakThread[3].Close();

	test.Printf(_L("total trace data processed = %ld k\n"),totalSize>>10);

	test.Next(_L("Restore buffer"));
	r = Trace.ResizeBuffer(oldSize);
	test_KErrNone(r);

	test.End();
	}


void KernelBenchmark(TUint& aTime1,TUint& aTime2,TUint& aTime3,TBool aTraceEnabled,TBool aFilter2)
	{
	TInt oldSize = Trace.BufferSize();

	TInt r = Trace.ResizeBuffer(0x1000);
	test_KErrNone(r);
	Trace.SetMode(RBTrace::EFreeRunning|RBTrace::EEnable);

	if(aFilter2)
		r = 1+TraceTest.TestBenchmark2(0,1000000);
	else
		r = 1+TraceTest.TestBenchmark(0,1000000);
	TUint8* data;
	TInt size = Trace.GetData(data);
	test(aTraceEnabled ? size!=0 : size==0);
	Trace.DataUsed();
	aTime1 = 1000000000/r;

	r = Trace.ResizeBuffer(0x1000);
	test_KErrNone(r);
	Trace.SetMode(RBTrace::EFreeRunning|RBTrace::EEnable);

	if(aFilter2)
		r = 1+TraceTest.TestBenchmark2(KMaxBTraceDataArray,1000000);
	else
		r = 1+TraceTest.TestBenchmark(KMaxBTraceDataArray,1000000);
	size = Trace.GetData(data);
	test(aTraceEnabled ? size!=0 : size==0);
	Trace.DataUsed();
	aTime2 = 1000000000/r;

	r = 1+TraceTest.TestBenchmarkCheckFilter(aFilter2,1000000);
	aTime3 = 1000000000/r;

	r = Trace.ResizeBuffer(oldSize);
	test_KErrNone(r);
	}


void UserBenchmark(TUint& aTime1,TUint& aTime2,TUint& aTime3,TBool aTraceEnabled,TBool aFilter2)
	{
	TInt oldSize = Trace.BufferSize();

	RTimer timer;
	TRequestStatus status;
	test_KErrNone(timer.CreateLocal());

	TInt r = Trace.ResizeBuffer(0x1000);
	test_KErrNone(r);
	Trace.SetMode(RBTrace::EFreeRunning|RBTrace::EEnable);

	timer.After(status,1);
	User::WaitForRequest(status);
	timer.After(status,1000000);
	r = 0;
	if(aFilter2)
		do
			{
			BTraceFiltered4(BTrace::ETest1,0,KBTraceFilterTestUid1);
			++r;
			}
		while(status==KRequestPending);
	else
		do
			{
			BTrace0(BTrace::ETest1,0);
			++r;
			}
		while(status==KRequestPending);
	User::WaitForRequest(status);
	TUint8* data;
	TInt size = Trace.GetData(data);
	test(aTraceEnabled ? size!=0 : size==0);
	Trace.DataUsed();
	aTime1 = 1000000000/r;

	r = Trace.ResizeBuffer(0x1000);
	test_KErrNone(r);
	Trace.SetMode(RBTrace::EFreeRunning|RBTrace::EEnable);

	timer.After(status,1);
	User::WaitForRequest(status);
	timer.After(status,1000000);
	r = 0;
	if(aFilter2)
		do
			{
			BTraceFilteredContextN(BTrace::ETest1,0,KBTraceFilterTestUid1,0,KTestTrace1,KMaxBTraceDataArray);
			++r;
			}
		while(status==KRequestPending);
	else
		do
			{
			BTraceContextN(BTrace::ETest1,0,0,0,KTestTrace1,KMaxBTraceDataArray);
			++r;
			}
		while(status==KRequestPending);
	User::WaitForRequest(status);
	size = Trace.GetData(data);
	test(aTraceEnabled ? size!=0 : size==0);
	Trace.DataUsed();
	aTime2 = 1000000000/r;

	timer.After(status,1);
	User::WaitForRequest(status);
	timer.After(status,1000000);
	r = 0;
	TBool check = -1;
	if(aFilter2)
		do
			{
			check = BTrace::CheckFilter2(BTrace::ETest1,KBTraceFilterTestUid1);
			++r;
			}
		while(status==KRequestPending);
	else
		do
			{
			check = BTrace::CheckFilter(BTrace::ETest1);
			++r;
			}
		while(status==KRequestPending);
	test(check == aTraceEnabled);
	aTime3 = 1000000000/r;

	r = Trace.ResizeBuffer(oldSize);
	test_KErrNone(r);
	}


//---------------------------------------------
//! @SYMTestCaseID KBASE-T_BTRACE-0066-0067
//! @SYMTestType UT
//! @SYMPREQ PREQ1030
//! @SYMTestCaseDesc Benchmark tracing.
//! @SYMTestActions Time the generation of minimum and maximum sized traces both with
//!		and without the trace filter enabled for the trace category.
//! @SYMTestExpectedResults Trace time with filter enabled should not excede 2000 nano-seconds,
//!		and time with filter disabled should not excede 500 nano-seconds. (These limits are not
//!		asserted). If time significantly excedes this then detailed investigation is required.
//! @SYMTestPriority Medium
//! @SYMTestStatus Implemented
//---------------------------------------------
void TestBenchmark(TBool aUserTrace)
	{
	TUint t1 = 0;
	TUint t2 = 0;
	TUint t3 = 0;

#define BENCH(a1,a2) aUserTrace ? UserBenchmark(t1,t2,t3,a1,a2) : KernelBenchmark(t1,t2,t3,a1,a2)

	test.Printf(_L("                                      Min Trace   Max Trace      Filter\n"));

	Trace.SetFilter(BTrace::ETest1,0);
	BENCH(0,0);
	test.Printf(_L("filter1 off                           %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter(BTrace::ETest1,1);
	BENCH(1,0);
	test.Printf(_L("filter1 on                            %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter(BTrace::ETest1,0);
	Trace.SetFilter2(0);
	BENCH(0,1);
	test.Printf(_L("filter1 off   filter2 off             %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter(BTrace::ETest1,1);
	Trace.SetFilter2(0);
	BENCH(0,1);
	test.Printf(_L("filter1 on    global filter2 off      %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter2(1);
	BENCH(1,1);
	test.Printf(_L("filter1 on    global filter2 on       %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter2(&KBTraceFilterTestUid2,1);
	BENCH(0,1);
	test.Printf(_L("filter1 on    1 UID filter2 off       %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter2(&KBTraceFilterTestUid1,1);
	BENCH(1,1);
	test.Printf(_L("filter1 on    1 UID filter2 on        %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter2(BigFilter2,sizeof(BigFilter2)/sizeof(TUint32));
	Trace.SetFilter2(KBTraceFilterTestUid1,0);
	BENCH(0,1);
	test.Printf(_L("filter1 on    100 UID filter2 off     %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter2(BigFilter2,sizeof(BigFilter2)/sizeof(TUint32));
	BENCH(1,1);
	test.Printf(_L("filter1 on    100 UID filter2 on      %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter(BTrace::ETest1,0);
	Trace.SetFilter2(0);
	}

struct THREADTRACETESTSTRUCT {
	TInt* alloc_addr;
	void* chunk_addr;
};

LOCAL_D TInt threadtraceTestThread(TAny* param)
	{
	THREADTRACETESTSTRUCT* p = (THREADTRACETESTSTRUCT*)param;
	p->alloc_addr = new TInt;
	delete p->alloc_addr;
	return 0;
	}

void TestHeapAndChunkTrace()
	{
	if(KErrNotSupported==Trace.Filter(BTrace::EHeap))
		{
		test.Start(_L("Trace category EHeap not supported by this build of kernel."));
		test.End();
		return;
		}

	test.Start(_L("Reset trace buffer"));
	Trace.SetMode(0);
	TInt oldSize = Trace.BufferSize();
	TInt r = Trace.ResizeBuffer(0x100000);
	test_KErrNone(r);
	Trace.SetFilter(BTrace::EHeap,0);
	Trace.SetFilter(BTrace::EChunks,0);
	Trace.SetMode(RBTrace::EEnable);

	test.Next(_L("Test heap-alloc works as expected"));
	Trace.SetFilter(BTrace::EHeap,1);
	TBool chunkTraceEnabled = Trace.SetFilter(BTrace::EChunks,1)!=KErrNotSupported;

	// Create a test thread
	THREADTRACETESTSTRUCT threadtest;
	RThread thread;
	TRequestStatus stat;
	test.Next(_L("Test thread creation heap trace"));
	r=thread.Create(_L("t_tbrace_2"),threadtraceTestThread,KDefaultStackSize,0x2000,0x2000,&threadtest);
	test_KErrNone(r);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);


	// search for heap records in trace...
	TUint8* data;
	TUint8* trace;
	TInt size=Trace.GetData(trace);
	test_NotNull(size);
	TUint8* end = trace+size;
	TUint chunk_ptr = 0, heap_chunk_ptr = 0;
	int found_chunk_create = 0;
	int found_heap_chunk_create = 0;
	int found_heap_create=0;
	int found_heap_alloc=0;
	int found_heap_free=0;
	for(data=trace; data<end; data=BTrace::NextRecord(data))
		{
		if(data[BTrace::ECategoryIndex]==BTrace::EChunks)
			{
			if(data[BTrace::ESubCategoryIndex]==BTrace::EChunkCreated)
				{
				found_chunk_create=1;
				chunk_ptr = Body(data)[0];
				}
			}
		if(data[BTrace::ECategoryIndex]==BTrace::EHeap)
			{
			if(data[BTrace::ESubCategoryIndex]==BTrace::EHeapCreate)
				found_heap_create=1;
			if(data[BTrace::ESubCategoryIndex]==BTrace::EHeapChunkCreate)
				{
				found_heap_chunk_create=1;
				heap_chunk_ptr = Body(data)[1];
				}
			if(data[BTrace::ESubCategoryIndex]==BTrace::EHeapAlloc)
				{
				if(Body(data)[1]==(TUint)threadtest.alloc_addr)
					{
					found_heap_alloc=1;
					test_Compare(Body(data)[2], >= ,4);
					test_Equal(4, Body(data)[3]);
					}
				}
			if(data[BTrace::ESubCategoryIndex]==BTrace::EHeapFree)
				{
				if(Body(data)[1]==(TUint)threadtest.alloc_addr)
					found_heap_free=1;
				}
			}
		}
	test(found_heap_create);
	test(found_heap_chunk_create);
	test(found_heap_alloc);
	test(found_heap_free);
	if(!chunkTraceEnabled)
		{
		test.Next(_L("Trace category EChunk not supported by this build of kernel."));
		test.End();
		return;
		}
	test(chunk_ptr && heap_chunk_ptr);
	test(chunk_ptr == heap_chunk_ptr);
	//Trace.DataUsed();

	// Turn on chunk trace and empty btrace buffer
	test.Next(_L("Chunk testing"));
	Trace.SetFilter(BTrace::EChunks,1);
	Trace.Empty();

	// Create a chunk and test the expected traces
	RChunk chunk;
	test_KErrNone(chunk.CreateLocal(4096, 4*1024*1024));
	trace = NULL;
	size=Trace.GetData(trace);
	test_NotNull(size);
	end = trace+size;
	found_chunk_create=0;
	for(data=trace; data<end; data=BTrace::NextRecord(data))
		{
		if(data[BTrace::ECategoryIndex]==BTrace::EChunks)
			{
			if(data[BTrace::ESubCategoryIndex]==BTrace::EChunkCreated)
				{
				test_Equal(4*1024*1024, Body(data)[1]);
				found_chunk_create = 1;
				}
			if(data[BTrace::ESubCategoryIndex]==BTrace::EChunkMemoryAllocated)
				{
				test_Equal(4096, Body(data)[2]);
				found_heap_alloc = 1;
				}
			}
		}
	test(found_heap_alloc && found_chunk_create);
	Trace.DataUsed();

	test.Next(_L("Cleanup after tests"));
	Trace.SetFilter(BTrace::EHeap,0);
	Trace.SetMode(0);
	r = Trace.ResizeBuffer(oldSize);
	test_KErrNone(r);

	test.End();
	}

void SetBTraceFilter(const TUint32* aNew,TUint32* aOld)
	{
	TUint category = 0;
	do
		{
		TUint32 newBits = *aNew++;
		TUint32 oldBits = 0;
		do
			{
			oldBits >>= 1;
			if(Trace.SetFilter(category,newBits&1))
				oldBits |= 0x80000000u;
			newBits >>= 1;
			++category;
			}
		while(category&31);
		if(aOld)
			*aOld++ = oldBits;
		}
	while(category<256);
	}


TUint32 OldTraceFilter[8] = {0};


//thread function to generate BTrace::EHeap::EHeapAllocFail event
LOCAL_D TInt ThreadtraceAllocFailTestThread(TAny* /*param*/)
  	{
  	RPointerArray<TInt32> array(1000);
  	TInt i = 1;
  	for(;;)
  		{
  		TInt32* p = new TInt32[(++i)*2];
  		if(!p)
  			break;
  		else
  			if(array.Append(p) == KErrNoMemory)
  				break;
  		}
  	array.ResetAndDestroy();
  	array.Close();
  	return 0;
  	}

//thread function to generate BTrace::EHeap::EHeapReAllocFail event
LOCAL_D TInt ThreadtraceReAllocFailTestThread(TAny* /*param*/)
	{
	TInt s = 4;
	TUint8 *p = (TUint8*)User::Alloc(s);
	for(;;)
		{
		s *= 2;
		TUint8* np = (TUint8*)User::ReAlloc(p, s);
		if(!np)
			{
			delete [] p;
			break;
			}
		else
			p=np;
		}
	return 0;
	}



//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-T_BTRACE-0733
//! @SYMTestCaseDesc			Test BTrace - EHeapAllocFail and EHeapReAllocFail subcategories.
//! @SYMTestType				UT
//! @SYMPREQ					PREQ1340
//! @SYMTestPriority			Medium
//! @SYMTestActions
//! 	1.	Configure BTrace to use EHeap and EThreadIdentification as main filter
//!			categories.
//! 	2.	Create and start a thread which will eventually exhaust
//!			all memory resources (this should result with sending a trace
//!			from allocator class - sent trace should have
//!			category - EHeap and subcategory - EHeapAllocFail).
//! 	3.	After thread (from previous point) crashes, get trace data from BTrace
//!			and search for trace with category EHeap and subcategory EHeapAllocFail
//!		4.	Validate trace data payload - it should have exactly 8 bytes (2x4 bytes values).
//!
//! @SYMTestExpectedResults
//! 	1.	Trace data should contain at least one trace with category
//!			EHeap and subcategory EHeapAllocFail.
//! 	2.	Trace data payload (where category is EHeap and subcategory is EHeapAllocFail) should
//!			be 8 bytes long (2 x 4 bytes values).
//---------------------------------------------------------------------------------------------------------------------
void TestHeapAllocFailEvent(TInt aTestCategory)
	{
	//configurnig trace
	if(aTestCategory == BTrace::EHeapAllocFail)
		test.Next(_L("Test to check BTrace::EHeap::EHeapAllocFail trace event"));
	else if(aTestCategory == BTrace::EHeapReAllocFail)
		test.Next(_L("Test to check BTrace::EHeap::EHeapReAllocFail trace event"));
	//configure BTrace
	Trace.Open();
	Trace.SetMode(0);
	TInt r = Trace.ResizeBuffer(0x100000);

	test_KErrNone(r);
	Trace.SetFilter(BTrace::EHeap,0);
	Trace.SetFilter(BTrace::EChunks,0);
	Trace.SetMode(RBTrace::EEnable);
	Trace.SetFilter(BTrace::EHeap,1);
	Trace.SetFilter(BTrace::EThreadIdentification,1);

	//start thread to cause EHeapAllocFail event
	RThread thread;
	TRequestStatus stat;

	//starting test thread
	if(aTestCategory == BTrace::EHeapAllocFail)
		r = thread.Create(_L("t_btrace_allocfail_thread"), ThreadtraceAllocFailTestThread, KDefaultStackSize, 0x2000, 0x2000, NULL);
	else if(aTestCategory == BTrace::EHeapReAllocFail)
		r = thread.Create(_L("t_btrace_reallocfail_thread"), ThreadtraceReAllocFailTestThread, KDefaultStackSize, 0x2000, 0x2000, NULL);

	test_KErrNone(r);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	thread.Close();

	//getting trace data
	TUint8* data;
	TUint8* trace;
	TInt size=Trace.GetData(trace);

	test_NotNull(size);
	TUint8* end = trace+size;
	TInt8 found_heap_allocfail = 0;

	TInt subCat = BTrace::EHeapAllocFail;
	TInt expectedDataSize = 8;
	if(aTestCategory == BTrace::EHeapReAllocFail)
		{
		subCat = BTrace::EHeapReAllocFail;
		expectedDataSize = 12;
		}

	for(data=trace; data<end; data=BTrace::NextRecord(data))
		{
		if(data[BTrace::ECategoryIndex]==BTrace::EHeap)
			if(data[BTrace::ESubCategoryIndex]==subCat)
				{
				found_heap_allocfail = 1;
				TInt allocfail_data_size = ((TInt)data[BTrace::ESizeIndex]) - ExtraSize(data);
				test_Equal(allocfail_data_size,expectedDataSize);
				break; //we're looking only for one record
				}
		}
	test(found_heap_allocfail);

	//closing trace
	Trace.DataUsed();
	Trace.Empty();
	Trace.SetMode(0);
	Trace.Close();
	}


bool FindHeapCorruptionTrace(TUint8* aData, TInt aSize, TInt& aFoundSize)
	{
	TUint8* end = aData+aSize;
	TUint8* data;
	for(data=aData; data<end; data=BTrace::NextRecord(data))
		{
		if(data[BTrace::ECategoryIndex] == BTrace::EHeap)
			if(data[BTrace::ESubCategoryIndex] == BTrace::EHeapCorruption)
				{
				aFoundSize = data[BTrace::ESizeIndex] - ExtraSize(data);
				return true;
				}
		}
	return false;
	}


//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-T_BTRACE-0734
//! @SYMTestCaseDesc			Test BTrace - EHeapCorruption subcategory.
//! @SYMTestType				UT
//! @SYMPREQ					PREQ1340
//! @SYMTestPriority			Medium
//! @SYMTestActions
//! 	1.	Configure BTrace to use EHeap and EThreadIdentification categories.
//! 	2.	Use test application (t_heapcorruption.exe) with parameters: 1, 2, 3 or 4 to
//!			to corrupt heap memory (for this application)
//! 	3.	Get trace data from BTrace and search for trace with category EHeap and
//!			subcategory EHeapCorruption.
//!		4.	Validate payload size for found traces.
//!
//! @SYMTestExpectedResults
//! 	1.	Test application (when started with either argument) should generate at least
//!			one trace with category EHeap and subcategory EHeapCorruption.
//! 	2.	Trace data payload should be 12 bytes long (3x4 bytes values).
//---------------------------------------------------------------------------------------------------------------------
void TestHeapCorruptionEvents()
	{
	TInt r = KErrNone;
	test.Next(_L("Test to check BTrace::EHeap::EHeapCorruption trace events"));
	//configure BTrace
	Trace.Open();
	Trace.SetMode(0);
	Trace.ResizeBuffer(0x100000);
	test_KErrNone(r);
	Trace.SetFilter(BTrace::EHeap,0);
	Trace.SetFilter(BTrace::EChunks,0);
	Trace.SetMode(RBTrace::EEnable);
	Trace.SetFilter(BTrace::EHeap,1);
	Trace.SetFilter(BTrace::EThreadIdentification,1);

	_LIT(KHeapCorrApp, "t_heapcorruption.exe");
	_LIT(KCorruption0, "0"); //RHeap:: corruption type (when EMonitorMemory flag is set)
	_LIT(KCorruption1, "1"); //RHeap::EBadFreeCellAddress corruption type
	_LIT(KCorruption2, "2"); //RHeap::EBadFreeCellSize corruption type
	_LIT(KCorruption3, "3"); //RHeap::EBadAllocatedCellSize corruption type
	_LIT(KCorruption4, "4"); //RHeap::EBadAllocatedCellAddress corruption type
	_LIT(KCorruptionSpecial1, "1000"); //RHeap::EBadAllocatedCellSize (with EMonitorMemory flag set)
	RProcess p;
	TInt e = KErrNone;
	TRequestStatus status;
	TUint8* trace;
	TInt size, payloadSize;
	bool res;

	//test 0 (for memory monitoring tools - when EMonitorMemory heap flag is set)
	test.Printf(_L("test 0\n"));
	Trace.SetMode(RBTrace::EEnable);
	e = p.Create(KHeapCorrApp, KCorruption0);
	test_KErrNone(e);
	p.Resume();
	p.Logon(status);
	User::WaitForRequest(status);
	p.Close();
	size = Trace.GetData(trace);
	test_NotNull(size);
	res = FindHeapCorruptionTrace(trace, size, payloadSize);
	test(res==true);
	test(payloadSize == 12); //payload in this case must be 12 bytes long (3x4bytes)
	Trace.DataUsed();
	Trace.Empty();
	Trace.SetMode(0);

	//test 1 -
	test.Printf(_L("test 1\n"));
	Trace.SetMode(RBTrace::EEnable);
	e = p.Create(KHeapCorrApp, KCorruption1);
	test_KErrNone(e);
	p.Resume();
	p.Logon(status);
	User::WaitForRequest(status);
	p.Close();
	size = Trace.GetData(trace);
	test_NotNull(size);
	res = FindHeapCorruptionTrace(trace, size, payloadSize);
	test(res==true);
	test(payloadSize == 12); //payload in this case must be 12 bytes long (3x4bytes)
	Trace.DataUsed();
	Trace.Empty();
	Trace.SetMode(0);

	//test 2
	test.Printf(_L("test 2\n"));
	Trace.SetMode(RBTrace::EEnable);
	e = p.Create(KHeapCorrApp, KCorruption2);
	test_KErrNone(e);
	p.Resume();
	p.Logon(status);
	User::WaitForRequest(status);
	p.Close();
	size = Trace.GetData(trace);
	test_NotNull(size);
	res = FindHeapCorruptionTrace(trace, size, payloadSize);
	test(res==true);
	test(payloadSize == 12); //payload in this case must be 12 bytes long (3x4bytes)
	Trace.DataUsed();
	Trace.Empty();
	Trace.SetMode(0);

	//test 3
	test.Printf(_L("test 3\n"));
	Trace.SetMode(RBTrace::EEnable);
	e = p.Create(KHeapCorrApp, KCorruption3);
	test_KErrNone(e);
	p.Resume();
	p.Logon(status);
	User::WaitForRequest(status);
	p.Close();
	size = Trace.GetData(trace);
	test_NotNull(size);
	res = FindHeapCorruptionTrace(trace, size, payloadSize);
	test(res==true);
	test(payloadSize == 12); //payload in this case must be 12 bytes long (3x4bytes)
	Trace.DataUsed();
	Trace.Empty();
	Trace.SetMode(0);

	//test 4
	test.Printf(_L("test 4\n"));
	Trace.SetMode(RBTrace::EEnable);
	e = p.Create(KHeapCorrApp, KCorruption4);
	test_KErrNone(e);
	p.Resume();
	p.Logon(status);
	User::WaitForRequest(status);
	p.Close();
	size = Trace.GetData(trace);
	test_NotNull(size);
	res = FindHeapCorruptionTrace(trace, size, payloadSize);
	test(res==true);
	test(payloadSize == 12); //payload in this case must be 12 bytes long (3x4bytes)
	Trace.DataUsed();
	Trace.Empty();
	Trace.SetMode(0);

	//test 5 (test ____MEMORY_MONITOR_CHECK_CELL macro)
	test.Printf(_L("test 5\n"));
	Trace.SetMode(RBTrace::EEnable);
	e = p.Create(KHeapCorrApp, KCorruptionSpecial1);
	test_KErrNone(e);
	p.Resume();
	p.Logon(status);
	User::WaitForRequest(status);
	p.Close();
	size = Trace.GetData(trace);
	test_NotNull(size);
	res = FindHeapCorruptionTrace(trace, size, payloadSize);
	test(res==true);
	test(payloadSize == 12); //payload in this case must be 12 bytes long (3x4bytes)
	Trace.DataUsed();
	Trace.Empty();
	Trace.SetMode(0);

	//closing trace
	Trace.DataUsed();
	Trace.Empty();
	Trace.SetMode(0);
	Trace.Close();
	}


//Set up utrace
TUint32 KUtracePcValues[3]={0, 0x123456, 0x987654};

#define T_UTRACE_HEADER(aSize,aClassification,aContext,aPc)																\
	((((aSize) + (aContext?4:0) + (aPc?4:0)) << BTrace::ESizeIndex*8)										\
	+(((aContext?BTrace::EContextIdPresent:0) | (aPc?BTrace::EPcPresent:0)) << BTrace::EFlagsIndex*8)			\
	+((aClassification) << BTrace::ECategoryIndex*8)																				\
	+((KTest1SubCategory) << BTrace::ESubCategoryIndex*8))

#define UTRACE_SECONDARY(aClassification,aModuleUid,aThreadIdPresent,aPcPresent,aPc,aFormatId)	\
	BTrace::OutFilteredPcFormatBig(T_UTRACE_HEADER(8,aClassification,aThreadIdPresent,aPcPresent),(TUint32)(aModuleUid),aPc,aFormatId,0,0)

#define UTRACE_SECONDARY_4(aClassification,aModuleUid,aThreadIdPresent,aPcPresent,aPc,aFormatId, aData1) \
	BTrace::OutFilteredPcFormatBig(T_UTRACE_HEADER(8,aClassification,aThreadIdPresent,aPcPresent),(TUint32)(aModuleUid),aPc,aFormatId,&aData1,4)

#define UTRACE_SECONDARY_ANY(aClassification, aModuleUid, aThreadIdPresent, aPcPresent, aPc, aFormatId, aData, aDataSize) \
	BTrace::OutFilteredPcFormatBig(T_UTRACE_HEADER(8,aClassification,aThreadIdPresent,aPcPresent),(TUint32)(aModuleUid),aPc,aFormatId,aData,(TInt)(aDataSize))

void UTraceUserBenchmark(TUint& aTime1,TUint& aTime2,TUint& aTime3,TBool aTraceEnabled)
	{
	TInt oldSize = Trace.BufferSize();

	RTimer timer;
	TRequestStatus status;
	test_KErrNone(timer.CreateLocal());

	TInt r = Trace.ResizeBuffer(0x1000);
	test_KErrNone(r);
	Trace.SetMode(RBTrace::EFreeRunning|RBTrace::EEnable);

	timer.After(status,1);
	User::WaitForRequest(status);
	timer.After(status,1000000);
	r = 0;
	TUint32* frm = (TUint32*)KTestTrace1;
	TUint16 formatId = (TUint16)frm[2];

	do
		{
		UTRACE_SECONDARY(BTrace::ETest1,KBTraceFilterTestUid1,EFalse,EFalse,0,formatId);
		++r;
		}
	while(status==KRequestPending);
	User::WaitForRequest(status);
	TUint8* data;
	TInt size = Trace.GetData(data);
	test(aTraceEnabled ? size!=0 : size==0);
	Trace.DataUsed();
	aTime1 = 1000000000/r;

	r = Trace.ResizeBuffer(0x1000);
	test_KErrNone(r);
	Trace.SetMode(RBTrace::EFreeRunning|RBTrace::EEnable);

	timer.After(status,1);
	User::WaitForRequest(status);
	timer.After(status,1000000);
	r = 0;
	do
		{
		//BTraceFilteredContextN(BTrace::ETest1,0,KBTraceFilterTestUid1,0,KTestTrace1,KMaxBTraceDataArray);
		UTRACE_SECONDARY_ANY(BTrace::ETest1,KBTraceFilterTestUid1,ETrue,EFalse,0,formatId,KTestTrace1,KMaxBTraceDataArray);
		++r;
		}
	while(status==KRequestPending);
	User::WaitForRequest(status);
	size = Trace.GetData(data);
	test(aTraceEnabled ? size!=0 : size==0);
	Trace.DataUsed();
	aTime2 = 1000000000/r;

	timer.After(status,1);
	User::WaitForRequest(status);
	timer.After(status,1000000);
	r = 0;
	TBool check = -1;
	do
		{
		check = BTrace::CheckFilter2(BTrace::ETest1,KBTraceFilterTestUid1);
		++r;
		}
	while(status==KRequestPending);
	test(check == aTraceEnabled);
	aTime3 = 1000000000/r;

	r = Trace.ResizeBuffer(oldSize);
	test_KErrNone(r);
	}

//---------------------------------------------
//! @SYMTestCaseID 		KBASE-T_BTRACE-2442
//! @SYMTestType 		UT
//! @SYMPREQ 			CR1623
//! @SYMTestCaseDesc 	Benchmark utracing.
//! @SYMTestActions 	Time the generation of minimum and maximum sized utraces both with
//!						and without the trace filter enabled for the trace category.
//! @SYMTestExpectedResults Trace time with filter enabled should not excede 2000 nano-seconds,
//!						and time with filter disabled should not excede 500 nano-seconds. (These limits are not
//!						asserted). If time significantly excedes this then detailed investigation is required.
//! @SYMTestPriority 	Medium
//! @SYMTestStatus 		Prototype
//---------------------------------------------
void TestUtraceBenchmark(TBool aUserTrace)
	{
	TUint t1 = 0;
	TUint t2 = 0;
	TUint t3 = 0;
//This uses btrace kernel benching atm, need to change once kernel is implemented
#define UBENCH(a1) aUserTrace ? UTraceUserBenchmark(t1,t2,t3,a1) : KernelBenchmark(t1,t2,t3,a1,1)

	test.Printf(_L("                                      Min Trace   Max Trace      Filter\n"));

	Trace.SetFilter(BTrace::ETest1,0);
	Trace.SetFilter2(0);
	UBENCH(0);
	test.Printf(_L("filter1 off   filter2 off             %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter(BTrace::ETest1,1);
	Trace.SetFilter2(0);
	UBENCH(0);
	test.Printf(_L("filter1 on    global filter2 off      %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter2(1);
	UBENCH(1);
	test.Printf(_L("filter1 on    global filter2 on       %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter2(&KBTraceFilterTestUid2,1);
	UBENCH(0);
	test.Printf(_L("filter1 on    1 UID filter2 off       %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter2(&KBTraceFilterTestUid1,1);
	UBENCH(1);
	test.Printf(_L("filter1 on    1 UID filter2 on        %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter2(BigFilter2,sizeof(BigFilter2)/sizeof(TUint32));
	Trace.SetFilter2(KBTraceFilterTestUid1,0);
	UBENCH(0);
	test.Printf(_L("filter1 on    100 UID filter2 off     %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter2(BigFilter2,sizeof(BigFilter2)/sizeof(TUint32));
	UBENCH(1);
	test.Printf(_L("filter1 on    100 UID filter2 on      %6d ns   %6d ns   %6d ns\n"),t1,t2,t3);

	Trace.SetFilter(BTrace::ETest1,0);
	Trace.SetFilter2(0);
	}


void CallUtrace(TBool aType, TBool aContextPresent, TBool aPcPresent, TInt aSizeOfData)
	{
	if(!(aType&RBTraceTest::EUserTrace))
		{
		// use driver to create a kernel trace...
		TraceTest.Trace(aType,KTestTrace1,aSizeOfData);
		return;
		}

	TUint32* data = (TUint32*)KTestTrace1;
	TUint16 formatId = (TUint16)data[2];
	//multiparted traces (depending on size)
	if(aSizeOfData <= 0)
		{
		UTRACE_SECONDARY(BTrace::ETest1, KBTraceFilterTestUid1, aContextPresent, aPcPresent, KUtracePcValues[1], formatId);
		UTRACE_SECONDARY(BTrace::ETest1, KBTraceFilterTestUid1, aContextPresent, aPcPresent, KUtracePcValues[2], formatId);
		}
	else if(aSizeOfData <= 4)
		{
		UTRACE_SECONDARY_4(BTrace::ETest1, KBTraceFilterTestUid1, aContextPresent, aPcPresent, KUtracePcValues[1], formatId, data[3]);
		UTRACE_SECONDARY_4(BTrace::ETest1, KBTraceFilterTestUid1, aContextPresent, aPcPresent, KUtracePcValues[2], formatId, data[3]);
		}
	else //aSizeOfData > 8
		{
		UTRACE_SECONDARY_ANY(BTrace::ETest1, KBTraceFilterTestUid1, aContextPresent, aPcPresent, KUtracePcValues[1], formatId, data+3, aSizeOfData);
		UTRACE_SECONDARY_ANY(BTrace::ETest1, KBTraceFilterTestUid1, aContextPresent, aPcPresent, KUtracePcValues[2], formatId, data+3, aSizeOfData);
		//BTraceFilteredContextPcBig(BTrace::ETest1,KTest1SubCategory,KBTraceFilterTestUid1,data+2,aSizeOfData-4);
		//BTraceFilteredContextPcBig(BTrace::ETest1,KTest1SubCategory,KBTraceFilterTestUid1,data+2,aSizeOfData-4);
		}
	}


void CheckUtraceFlags(TUint8* aData, TUint8* aData2, TBool aContextPresent, TBool aPcPresent)
	{
	TUint offset = ContextOffset(aData);
	if(aContextPresent)
		{
		aContextPresent = BTrace::EContextIdPresent;
		test((aContextPresent == (aData[BTrace::EFlagsIndex]&BTrace::EContextIdPresent)));
		test_Equal(ThisTraceContextId, ((TUint32*)(aData+offset))[0] );
		test_Equal(ThisTraceContextId, ((TUint32*)(aData2+offset))[0] );
		}
	if(aPcPresent)
		{
		aPcPresent = BTrace::EPcPresent;
		test((aPcPresent == (aData[BTrace::EFlagsIndex]&BTrace::EPcPresent)));
		if(aContextPresent)
			{
			test_Compare(((TUint32*)(aData+offset))[1], ==, KUtracePcValues[1]); //(offset plus 1 because context id is before pc)
			test_Compare(((TUint32*)(aData2+offset))[1], ==, KUtracePcValues[2]);
			}
		else
			{
			test_Compare(((TUint32*)(aData+offset))[0], ==, KUtracePcValues[1]);
			test_Compare(((TUint32*)(aData2+offset))[0], ==, KUtracePcValues[2]);
			}
		}
	}

void SetupTestDataForUtrace(TBool aChangeData = ETrue)
	{
	if(aChangeData)//change to accomodate iFormatId only being 16 bit
		{
			KTestTrace1[10] = (TUint8) 0;
			KTestTrace1[11] = (TUint8) 0;
		}
	else //restore to original data
		{
		KTestTrace1[10] = (TUint8) 10;
		KTestTrace1[11] = (TUint8) 11;
		}
	}

//---------------------------------------------
//! @SYMTestCaseID 			KBASE-T_BTRACE-2441
//! @SYMTestType 			UT
//! @SYMPREQ 				CR1623
//! @SYMTestCaseDesc 		Test UTraces, including multiparted utraces, only testing user side for now, kernel side will be added later
//! @SYMTestActions			Generate traces from kernel code using the UTrace macros as defined above,
//! @SYMTestExpectedResults Traces where broken down into multiple parts and
//!							all trace contents captured by RBTrace matched those specified
//!							at point of trace generation. Also, where appropriate, PC and/or
//!							Context ID values are present and correct.
//! @SYMTestPriority 		High
//! @SYMTestStatus 			Prototype
//---------------------------------------------
void TestUtrace(TBool aUserTrace, TBool aFilter2)
	{
	test.Next(_L("Testing utrace\n"));
	TInt oldSize = Trace.BufferSize();
	TInt r = Trace.ResizeBuffer(0x100000);
	test_KErrNone(r);
	Trace.SetMode(RBTrace::EEnable);

	//set up the test data for UTrace
	SetupTestDataForUtrace();

	// dummy trace do get current thread context id
	Trace.SetFilter(BTrace::ETest1,0);
	ThisTraceContextId = TraceTest.Trace(BTrace::EContextIdPresent,KTestTrace1,0);

	// create a test filter...
	TInt extraFlags = 0;
	if(aUserTrace)
		extraFlags |= RBTraceTest::EUserTrace;
	TInt minSize = 4;
	if(aFilter2)
		{
		extraFlags |= RBTraceTest::EFilter2Trace;
		minSize += 4;
		}

	//Test that filtering works...
	TInt filterMode;
	for(filterMode=0; filterMode<(aFilter2?6:2); ++filterMode)
		{

		// setup filters...
		Trace.SetFilter(BTrace::ETest1,1);
		Trace.SetFilter2(BigFilter2,KNumBTraceFilterTestUids);
		if(filterMode==0 || filterMode==2)
			Trace.SetFilter(BTrace::ETest1,0); // disable in primary filter
		if(filterMode==0 || filterMode==1)
			Trace.SetFilter2(KBTraceFilterTestUid1,0); // disable in secondary filter
		if(filterMode==4)
			Trace.SetFilter2(0); // disable entire secondary filter
		if(filterMode==5)
			Trace.SetFilter2(1); // enable entire secondary filter

		// expectTrace is true if we expect trace to be output...
		TBool expectTrace = aFilter2 ? (filterMode==3 || filterMode==5) : filterMode&1;

		switch(filterMode)
			{
		case 0: test.Next(_L("Test with primary filter OFF, secondary filter OFF")); break;
		case 1: test.Next(_L("Test with primary filter ON, secondary filter OFF")); break;
		case 2: test.Next(_L("Test with primary filter OFF, secondary filter ON")); break;
		case 3: test.Next(_L("Test with primary filter ON, secondary filter ON")); break;
		case 4: test.Next(_L("Test with primary filter ON, global secondary filter OFF")); break;
		case 5: test.Next(_L("Test with primary filter ON, global secondary filter ON")); break;
			}

		TBool contextPresent = EFalse;
		TBool pcPresent = EFalse;
		for(TInt flags = 0; flags < 4; flags++)
			{
			switch(flags)
				{
				case 0:	test.Printf(_L("ContextId OFF, Pc OFF\n")); break;
				case 1: contextPresent = ETrue; extraFlags = BTrace::EContextIdPresent|BTrace::EPcPresent|extraFlags; test.Printf(_L("ContextId ON, Pc OFF\n"));break;
				case 2: pcPresent = ETrue; extraFlags = BTrace::EContextIdPresent|BTrace::EPcPresent|extraFlags; test.Printf(_L("ContextId OFF, Pc ON\n"));break;
				case 3: contextPresent = ETrue; pcPresent = ETrue; extraFlags = BTrace::EContextIdPresent|BTrace::EPcPresent|extraFlags; test.Printf(_L("ContextId ON, Pc ON\n"));break;
				}

			// multiparted traces...
			BigTraceBeginTest();
			for(TInt i=0; i<=(TInt)sizeof(KTestTrace1)-4; i++)
				{
				//CallUtrace(typeOfCall, contextPresent, pcPresent, i);
				CallUtrace(extraFlags, contextPresent, pcPresent, i);

				TUint8* data;
				TUint8* data2;
				TInt size = Trace.GetData(data);
				if(!expectTrace)
					{
					test(!size);
					continue;
					}
				size /= 2;
				data2 = data+size;

				TInt sizePlusFormatId = i + 8;
				test(CheckBigTrace1(data, size, sizePlusFormatId, KTest1SubCategory, 4));
				test(CheckBigTrace1(data2, size, sizePlusFormatId, KTest1SubCategory, 4));

				CheckUtraceFlags(data, data2, contextPresent, pcPresent);
				Trace.DataUsed();
				}
			test_Equal(expectTrace,BigTraceEndTest()); // check we actually got multipart traces
			}
		}

	//Restore trace data
	SetupTestDataForUtrace(EFalse);

	test.Next(_L("Restore buffer\n"));
	r = Trace.ResizeBuffer(oldSize);
	test_KErrNone(r);
	Trace.SetFilter2(0);
	}



GLDEF_C TInt E32Main()
    {
	// initialise test trace data...
	KTestTrace1[0] = BTrace::ETest1;
	TUint i;
	for(i=8; i<sizeof(KTestTrace1); i++)
		{
		KTestTrace1[i] = (TUint8)i;
		KTestTrace2[i] = (TUint8)~i;
		}
	KTestTrace1[4] = (KBTraceFilterTestUid1>>0)&0xff;
	KTestTrace1[5] = (KBTraceFilterTestUid1>>8)&0xff;
	KTestTrace1[6] = (KBTraceFilterTestUid1>>16)&0xff;
	KTestTrace1[7] = (KBTraceFilterTestUid1>>24)&0xff;

	KTestTrace2[4] = (KBTraceFilterTestUid2>>0)&0xff;
	KTestTrace2[5] = (KBTraceFilterTestUid2>>8)&0xff;
	KTestTrace2[6] = (KBTraceFilterTestUid2>>16)&0xff;
	KTestTrace2[7] = (KBTraceFilterTestUid2>>24)&0xff;

	for(i=0; i<KNumBTraceFilterTestUids; i++)
		BigFilter2[i] = KBTraceFilterTestUid+i;

	test.Title();
	TInt r;

	test.Start(_L("Open LDD"));
	r = Trace.Open();
	test_KErrNone(r);

	test.Next(_L("Open test LDD"));
	r = User::LoadLogicalDevice(RBTraceTest::Name());
	test(r==KErrNone || r==KErrAlreadyExists);
	r = TraceTest.Open();
	test_KErrNone(r);

	// save trace settings...
	TUint savedMode = Trace.Mode();
	SetBTraceFilter(OldTraceFilter,OldTraceFilter);

	TBuf<256> commandLine;
	User::CommandLine(commandLine);
	if(commandLine==_L("dump"))
		{
		test.Next(_L("Dumping trace buffer..."));
		DumpTrace();
		goto done;
		}
	else if(commandLine==_L("soak"))
		{
		test.Next(_L("Running endless soak test..."));
		for(;;)
			{
			test.Next(_L("Soak test in free-running mode"));
			TestSoak(RBTrace::EFreeRunning,60);
			test.Next(_L("Soak test in sample mode"));
			TestSoak(0,60);
			}
		}

	test.Next(_L("Basic tests in sample mode"));
	TestBasics(0);
	test.Next(_L("Basic tests in free-running mode"));
	TestBasics(RBTrace::EFreeRunning);
	test.Next(_L("User BTrace test in sample mode"));
	TestUserTrace(0);
	test.Next(_L("Check secondary filter"));
	TestFilter2();

	test.Next(_L("Test user traces"));
	TestTrace(ETrue,EFalse);
	test.Next(_L("Test kernel traces"));
	TestTrace(EFalse,EFalse);
	test.Next(_L("Test user traces using secondary filter"));
	TestTrace(ETrue,ETrue);
	test.Next(_L("Test kernel traces using secondary filter"));
	TestTrace(EFalse,ETrue);

	test.Next(_L("Big user traces"));
	TestBig(ETrue,EFalse);
	test.Next(_L("Big kernel traces"));
	TestBig(EFalse,EFalse);
	test.Next(_L("Big user traces using secondary filter"));
	TestBig(ETrue,ETrue);
	test.Next(_L("Big kernel traces using secondary filter"));
	TestBig(EFalse,ETrue);

	test.Next(_L("Test category EThreadIdentification"));
	TestThreadIdentification();
	test.Next(_L("Test category ECpuUsage"));
	TestCpuUsage();

	test.Next(_L("Soak test in sample mode"));
	TestSoak(0,10);
	test.Next(_L("Soak test in free-running mode"));
	TestSoak(RBTrace::EFreeRunning,10);

	test.Next(_L("Benchmark kernel tracing"));
	TestBenchmark(0);
	test.Next(_L("Benchmark user tracing"));
	TestBenchmark(1);

	test.Next(_L("Test category EHeap"));
	TestHeapAndChunkTrace();

	test.Next(_L("Test user side utraces"));
	TestUtrace(ETrue, ETrue);
	//Kernel side is currently not implemented
	//test.Next(_L("Test kernel side utraces"));
	//TestUtrace(EFalse, ETrue);
	test.Next(_L("Benchmark user side UTraces"));
	TestUtraceBenchmark(ETrue);

done:
	// restore trace settin	gs...
	Trace.SetMode(0);
	SetBTraceFilter(OldTraceFilter,OldTraceFilter);
	Trace.SetMode(savedMode);

	test.Next(_L("Close LDD"));
	Trace.Close();
	TraceTest.Close();
	User::FreeLogicalDevice(RBTraceTest::Name());

	//test RHeap instrumentation:
	//with BTrace:EHeap:EHeapAllocFail trace points
	TestHeapAllocFailEvent(BTrace::EHeapAllocFail);
	//with BTrace:EHeap:EHeapReAllocFail trace points
	TestHeapAllocFailEvent(BTrace::EHeapReAllocFail);
	//with BTrace:EHeap:EHeapCorruption trace points
	TestHeapCorruptionEvents();

	test.End();
	return(0);
    }

