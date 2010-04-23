// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\nkernsa\utils.h
// 
//

#ifndef __UTILS_H__
#define __UTILS_H__
#include <e32err.h>
#include <nkern.h>

extern "C" {

extern TBool InitialThreadDefined;

extern void SetupMemoryAllocator();
extern TAny* malloc(TUint32 aSize);
extern void free(TAny* aCell);
extern TAny* realloc(TAny* aCell, TUint32 aSize);

extern TUint32 round_to_page(TUint32 x);

extern void SpinWait(TUint32 aTicks);
extern void nfcfspin(TUint32 aTicks);
extern void fcfspin(TUint64 aTicks);
extern void DebugPrint(const char*, int);

extern void __cpu_idle();
extern void __cpu_yield();
extern TLinAddr __stack_pointer();
extern TUint32 __cpu_status_reg();
extern TUint32 __cpu_id();
extern TUint32 __trace_cpu_num();
extern void __finish();

extern void* memcpy(void*, const void*, unsigned);
extern void* memset(void*, int, unsigned);
extern void* memclr(void*, unsigned);
extern void* wordmove(void*, const void*, unsigned);

extern TUint32 random(TUint32* aSeed);
extern void setup_block(TUint32* aBlock, TInt aNumWords);
extern TBool verify_block(const TUint32* aBlock, TInt aNumWords);
extern TBool verify_block_no_trace(const TUint32* aBlock, TInt aNumWords);
extern void setup_block_cpu(TUint32* aBlock, TInt aNumWords);
extern TInt verify_block_cpu_no_trace(const TUint32* aBlock, TInt aNumWords);

extern TUint64 fast_counter();
extern TUint64 fast_counter_freq();
extern TUint32 norm_fast_counter();			// normalised to count between 1-2MHz
extern TUint32 norm_fast_counter_freq();	// normalised to count between 1-2MHz
extern void init_fast_counter();
extern TInt __microseconds_to_fast_counter(TInt us);
extern TInt __microseconds_to_norm_fast_counter(TInt us);

extern void DumpMemory(const char* msg, const void* data, int length);
}

#define TEST_PRINT(s)	\
	DEBUGPRINT("NKTEST: " s)
#define TEST_PRINT1(s, a)	\
	DEBUGPRINT("NKTEST: " s, a)
#define TEST_PRINT2(s, a, b)	\
	DEBUGPRINT("NKTEST: " s, a, b)
#define TEST_PRINT3(s, a, b, c)	\
	DEBUGPRINT("NKTEST: " s, a, b, c)
#define TEST_PRINT4(s, a, b, c, d)	\
	DEBUGPRINT("NKTEST: " s, a, b, c, d)
#define TEST_PRINT5(s, a, b, c, d, e)	\
	DEBUGPRINT("NKTEST: " s, a, b, c, d, e)
#define TEST_PRINT6(s, a, b, c, d, e, f)	\
	DEBUGPRINT("NKTEST: " s, a, b, c, d, e, f)
#define TEST_PRINT7(s, a, b, c, d, e, f, g)	\
	DEBUGPRINT("NKTEST: " s, a, b, c, d, e, f, g)
#define TRACE_LINE()	\
	DEBUGPRINT("NKTEST: line %d", __LINE__)

#define	TEST_RESULT(x, s)	\
	do { if (!(x)) {DEBUGPRINT("NKTEST: " s); DEBUGPRINT("Line %d File %s", __LINE__, __FILE__);}} while(0)
#define	TEST_RESULT1(x, s, a)	\
	do { if (!(x)) {DEBUGPRINT("NKTEST: " s, a); DEBUGPRINT("Line %d File %s", __LINE__, __FILE__);}} while(0)
#define	TEST_RESULT2(x, s, a, b)	\
	do { if (!(x)) {DEBUGPRINT("NKTEST: " s, a, b); DEBUGPRINT("Line %d File %s", __LINE__, __FILE__);}} while(0)
#define	TEST_RESULT3(x, s, a, b, c)	\
	do { if (!(x)) {DEBUGPRINT("NKTEST: " s, a, b, c); DEBUGPRINT("Line %d File %s", __LINE__, __FILE__);}} while(0)
#define	TEST_RESULT4(x, s, a, b, c, d)	\
	do { if (!(x)) {DEBUGPRINT("NKTEST: " s, a, b, c, d); DEBUGPRINT("Line %d File %s", __LINE__, __FILE__);}} while(0)

#define TEST_OOM(p)	TEST_RESULT(p, "Out of memory");

#define RANGE_CHECK(l,x,h)	((l)<=(x) && (x)<=(h))
#define RANGE_LQ(x,h)	((x)<=(h))

#ifdef __SMP__
#define	for_each_cpu(cpu)	\
for((cpu)=0; (cpu)<NKern::NumberOfCpus(); ++(cpu))
#else
#define	for_each_cpu(cpu)	\
	for((cpu)=0; (cpu)<1; ++(cpu))
#endif

class CircBuf
	{
public:
	static CircBuf* New(TInt aSlots);
	CircBuf();
	~CircBuf();
	TInt TryGet(TUint32& aOut);
	TInt TryPut(TUint32 aIn);
	TUint32 Get();
	void Put(TUint32 aIn);
	void Reset();
	TInt Count();
public:
	TUint32* iBufBase;
	TInt iSlotCount;
	TInt iGetIndex;
	TInt iPutIndex;
	TSpinLock iLock;
	};

#endif


