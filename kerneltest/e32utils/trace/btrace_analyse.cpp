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
//

#if defined(__EPOC32__) || defined(__WINS__)
// compiling for Symbian OS...
#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <e32def.h>
#include <e32def_private.h>
#include <d32btrace.h>
#include "e32btrace.h"

inline void* malloc(TInt aSize) { return User::Alloc(aSize); }
inline void* realloc(void* aPtr,TInt aSize) { return User::ReAlloc(aPtr,aSize); }
inline void free(void* aPtr) { User::Free(aPtr); }

TUint8 PrintfBuffer[256];
TUint PrintfBufferWidth = 0;

static void printf(const char* aFormat,...)
	{
	VA_LIST list;
	VA_START(list,aFormat);
	TPtrC8 format((const TUint8*)aFormat);
	TPtr8 buf(PrintfBuffer+PrintfBufferWidth,sizeof(PrintfBuffer)-PrintfBufferWidth);
	buf.AppendFormatList(format,list);
	PrintfBufferWidth += buf.Size();
	for(;;)
		{
		TUint width = 0;
		for(;;)
			{
			if(width>=PrintfBufferWidth)
				return;
			if(PrintfBuffer[width]=='\n')
				break;
			++width;
			}
		TPtrC8 line(PrintfBuffer,width);
		++width;
		RDebug::RawPrint(line);
		_LIT(KLineEnd,"\r\n");
		RDebug::RawPrint(KLineEnd);
		memcpy(PrintfBuffer,PrintfBuffer+width,PrintfBufferWidth-width);
		PrintfBufferWidth -= width;
		}
	}

typedef TUint uintptr_t;

#ifndef ASSERT
#define ASSERT(c) (void)((c)||(AssertFailed(__LINE__)))
TInt AssertFailed(TInt aLine)
	{
	_LIT(KPanicCategory,"BTRACE-ANALYSE");
	User::Panic(KPanicCategory,aLine);
	return 0;
	}
#endif // ASSERT


//
//	Trace buffer helper functions - for use on target only
//
RBTrace Trace;
TUint8* AnalyseData;
TUint AnalyseDataRemain = 0;

void RePrime()
	{
	for(TInt i=0; i<256; ++i)
		{
		if(Trace.Filter(i))
			{
			// toggle filter to force a prime
			Trace.SetFilter(i,0);
			Trace.SetFilter(i,1);
			}
		}
	}

TUint AnalyseStream(TAny* aBuffer, TUint aMaxSize)
	{
	TUint size = AnalyseDataRemain;
	if(!size)
		{
		Trace.DataUsed();
		AnalyseDataRemain = Trace.GetData(AnalyseData);
		size = AnalyseDataRemain;
		}
	if(size>aMaxSize)
		size = aMaxSize;
	memcpy(aBuffer,AnalyseData,size);
	AnalyseData += size;
	AnalyseDataRemain -= size;
	return size;
	}

void ProcessAllTrace(TUint (*aInput)(TAny* aBuffer, TUint aMaxSize),TInt aReportLevel);

void DoAnalyse(TInt aAnalysisLevel)
	{
	AnalyseDataRemain = 0;
	TUint oldMode = Trace.Mode();
	Trace.SetMode(0); // turn off trace capture while we dump
	ProcessAllTrace(AnalyseStream, aAnalysisLevel);
	Trace.SetMode(oldMode);
	RePrime();
	}

TInt BTraceAnalyseSetup()
	{
	TInt r = Trace.Open();
	if (r != KErrNone)
		{
		return r;
		}
	// Stop tracing
	TUint oldMode = Trace.Mode();
	Trace.SetMode(0);

	// empty btrace buffer and reprime it
	Trace.Empty();
	Trace.SetMode(oldMode);
	RePrime();
	return KErrNone;
	}

void BTraceAnalyseEnd()
	{
	Trace.Close();
	}

void BTraceAnalyse(TInt aAnalysisLevel)
	{
	// Stop tracing
	TUint oldMode = Trace.Mode();
	Trace.SetMode(0);

	AnalyseDataRemain = 0;
	ProcessAllTrace(AnalyseStream, aAnalysisLevel);

	// empty btrace buffer and reprime it
	Trace.Empty();
	Trace.SetMode(oldMode);
	RePrime();
	}

#else
// compiling for host...

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#if defined(_MSC_VER)
typedef __int64 longlong;
typedef unsigned __int64 ulonglong;
#define BREAKPOINT { _asm int 3 }			/**< Invoke debugger */
#else
typedef long long longlong;
typedef long long ulonglong;
#define BREAKPOINT
#endif

typedef signed char			TInt8;
typedef unsigned char		TUint8;
typedef unsigned short		TUint16;
typedef unsigned int		TUint32;
typedef ulonglong			TUint64;
typedef unsigned int		uintptr_t;
typedef int					TInt;
typedef unsigned			TUint;
typedef int					TBool;
typedef void				TAny;
typedef float				TReal;
#define IMPORT_C
#include "e32btrace.h"

#define ASSERT(c) (void)((c)||(AssertFailed(__LINE__)))
extern "C" void exit(int);
TInt AssertFailed(TInt aLine)
	{
	fprintf(stderr,"Panic: BTRACE-ANALYSE %d",aLine);
	BREAKPOINT;
	exit(2);
	return 0;
	}

#define __UHEAP_MARK
#define __UHEAP_MARKEND

TAny* operator new(size_t, TAny* p) {return p;}

#endif // SYMBIAN_OS



//
// Global data
//

TInt ReportLevel = 0;
TUint8 TraceBuffer[0x10000];
TUint TraceBufferSize = 0;
TUint64	Timestamp = 0;
TUint64	TimestampBase = 0;
TUint32	TraceRecordId = 0;
TUint32	TimestampPeriod = 0;
TUint32	Timestamp2Period = 0;
TBool Timestamp64Bit = 0;
TUint TraceFormatErrors = 0;
TBool TraceBufferFilled = false;
TBool SMP = true;
TBool ErrorOnThisTrace = false;

const TUint8 KJunkTraceSubcategory = 255; // Dummy sub-category for EMetaTrace

const TInt KMaxCpus = 8;

//
// Utilities
//

void ToHex(TUint8* aString,TUint32 aValue)
	{
	TUint i=32;
	do
		{
		i -= 4;
		TUint8 c = (TUint8)((aValue>>i)&0xf);
		if(c>=10)
			c += 'a'-10;
		else
			c += '0';
		*aString++ = c;
		}
	while(i);
	}


TUint MakeName(TUint8* aString,const char* aName,TUint32 aHexValue)
	{
	TUint8* start = aString;
	for(;;)
		{
		TUint8 c = *aName++;
		if(!c)
			break;
		*aString++ = c;
		}
	ToHex(aString,aHexValue);
	aString[8] = 0;
	return aString-start+8;
	}


/**
Convert a timestamp into real time units (micro-seconds)
*/
TUint32 Time(TUint64 aTimestamp)
	{
	if(!TimestampPeriod)
		return (TUint32)aTimestamp;
	TInt exponent = (TInt8)(TimestampPeriod>>24);
	TUint64 mantissa = TimestampPeriod&0xffffff;
	aTimestamp *= mantissa;
	exponent += 32;
	if(exponent<0)
		aTimestamp >>= -exponent;
	else
		aTimestamp <<= exponent;
	TUint64 timeLo = (aTimestamp&0xffffffffu)*1000000;
	TUint64 timeHi = (aTimestamp>>32)*1000000;
	TUint64 us = timeHi+(timeLo>>32)+((timeLo>>31)&1);
	return TUint32(us);
	}


void ReportTimeUnits()
	{
	if(!TimestampPeriod)
		printf("\nAll times are given in BTrace Timestamp1 units.\n\n");
	else
		{
		TInt exponent = (TInt8)(TimestampPeriod>>24);
		TUint64 mantissa = TimestampPeriod&0xffffff;
		TUint64 period = 1000000;
		period *= mantissa;
		exponent += 32;
		if(exponent<0)
			period >>= -exponent;
		else
			period <<= exponent;
		printf("\nAll times are given in microseconds, resolution %d.%03dus\n\n",(int)TUint32(period>>32),(int)TUint32((((period&0xffffffffu)*1000)+0x80000000u)>>32));
		}
	}


TUint SetBits(TUint8* aData, TUint aOffset, TUint aSize)
	{
	TUint mask = 1<<(aOffset&7);
	TUint8* data = aData+(aOffset>>3);
	TUint errors = 0;
	while(aSize)
		{
		if(*data&mask)
			++errors;
		*data |= (TUint8)mask;
		mask <<= 1;
		if(mask>0xFF)
			{
			mask = 0x01;
			++data;
			}
		--aSize;
		}
	return errors;
	}


TUint ClearBits(TUint8* aData, TUint aOffset, TUint aSize)
	{
	TUint mask = 1<<(aOffset&7);
	TUint8* data = aData+(aOffset>>3);
	TUint errors = 0;
	while(aSize)
		{
		if(!(*data&mask))
			++errors;
		*data &= (TUint8)~mask;
		mask <<= 1;
		if(mask>0xFF)
			{
			mask = 0x01;
			++data;
			}
		--aSize;
		}
	return errors;
	}


void WarnIfError(TUint aErrorCount)
	{
	if (TraceBufferFilled)
		printf("WARNING - BTRACE BUFFER IS FULL SO TRACE DATA MAY BE INCOMPLETE\n");

	if(aErrorCount==0 && TraceFormatErrors==0)
		return;
	printf("CONSISTENCY ERRORS FOUND DURING TRACE ANALYSIS, RESULTS ARE UNRELIABLE!\n");
	}


#define CHECK_TRACE_DATA_WORDS(numWords)	\
	if(aTrace.iDataSize<numWords*4 && ++TraceFormatErrors) return


//
// Category naming
//

const char* const UnknownNames[256] =
	{
	"?00","?01","?02","?03","?04","?05","?06","?07","?08","?09",
	"?10","?11","?12","?13","?14","?15","?16","?17","?18","?19",
	"?20","?21","?22","?23","?24","?25","?26","?27","?28","?29",
	"?30","?31","?32","?33","?34","?35","?36","?37","?38","?39",
	"?40","?41","?42","?43","?44","?45","?46","?47","?48","?49",
	"?50","?51","?52","?53","?54","?55","?56","?57","?58","?59",
	"?60","?61","?62","?63","?64","?65","?66","?67","?68","?69",
	"?70","?71","?72","?73","?74","?75","?76","?77","?78","?79",
	"?80","?81","?82","?83","?84","?85","?86","?87","?88","?89",
	"?90","?91","?92","?93","?94","?95","?96","?97","?98","?99",
	"?100","?101","?102","?103","?104","?105","?106","?107","?108","?109",
	"?110","?111","?112","?113","?114","?115","?116","?117","?118","?119",
	"?120","?121","?122","?123","?124","?125","?126","?127","?128","?129",
	"?130","?131","?132","?133","?134","?135","?136","?137","?138","?139",
	"?140","?141","?142","?143","?144","?145","?146","?147","?148","?149",
	"?150","?151","?152","?153","?154","?155","?156","?157","?158","?159",
	"?160","?161","?162","?163","?164","?165","?166","?167","?168","?169",
	"?170","?171","?172","?173","?174","?175","?176","?177","?178","?179",
	"?180","?181","?182","?183","?184","?185","?186","?187","?188","?189",
	"?190","?191","?192","?193","?194","?195","?196","?197","?198","?199",
	"?200","?201","?202","?203","?204","?205","?206","?207","?208","?209",
	"?210","?211","?212","?213","?214","?215","?216","?217","?218","?219",
	"?220","?221","?222","?223","?224","?225","?226","?227","?228","?229",
	"?230","?231","?232","?233","?234","?235","?236","?237","?238","?239",
	"?240","?241","?242","?243","?244","?245","?246","?247","?248","?249",
	"?250","?251","?252","?253","?254","?255"
	};


#define STRINGIFY2(a) #a						/**< Helper for #STRINGIFY */
#define STRINGIFY(a) STRINGIFY2(a)				/**< Convert \a a into a quoted string */
#define CASE_CAT_NAME(name) case BTrace::name: return STRINGIFY(name)

const char* CategoryName(TUint8 aCategory)
	{
	switch((BTrace::TCategory)aCategory)
		{
		CASE_CAT_NAME(ERDebugPrintf);
		CASE_CAT_NAME(EKernPrintf);
		CASE_CAT_NAME(EPlatsecPrintf);
		case BTrace::EThreadIdentification: return "EThreadId"; //	CASE_CAT_NAME(EThreadIdentification)
		CASE_CAT_NAME(ECpuUsage);
    	CASE_CAT_NAME(EKernPerfLog);
    	CASE_CAT_NAME(EClientServer);
    	CASE_CAT_NAME(ERequests);
    	CASE_CAT_NAME(EChunks);
    	CASE_CAT_NAME(ECodeSegs);
		CASE_CAT_NAME(EPaging);
		CASE_CAT_NAME(EThreadPriority);
		CASE_CAT_NAME(EPagingMedia);
		CASE_CAT_NAME(EKernelMemory);
		CASE_CAT_NAME(EHeap);
		CASE_CAT_NAME(EMetaTrace);

		CASE_CAT_NAME(EFastMutex);
		CASE_CAT_NAME(EProfiling);
		CASE_CAT_NAME(ESymbianKernelSync);
		CASE_CAT_NAME(EFlexibleMemModel);
		CASE_CAT_NAME(EHSched);
		CASE_CAT_NAME(ETest1);
		CASE_CAT_NAME(ETest2);
		default:
			break;
		}
	return UnknownNames[aCategory];
	}

const char* SubCategoryName(TUint8 aCategory, TUint8 aSubCategory)
	{
	switch(aCategory)
		{
	case BTrace::ERDebugPrintf:
	case BTrace::EKernPrintf:
	case BTrace::EPlatsecPrintf:
		return ""; // no subcategories for these

	case BTrace::EThreadIdentification:
		switch((BTrace::TThreadIdentification)aSubCategory)
			{
		CASE_CAT_NAME(ENanoThreadCreate);
		CASE_CAT_NAME(ENanoThreadDestroy);
		CASE_CAT_NAME(EThreadCreate);
		CASE_CAT_NAME(EThreadDestroy);
		CASE_CAT_NAME(EThreadName);
		CASE_CAT_NAME(EProcessName);
		CASE_CAT_NAME(EThreadId);
		CASE_CAT_NAME(EProcessCreate);
		CASE_CAT_NAME(EProcessDestroy);
			}
		break;

	case BTrace::ECpuUsage:
		switch((BTrace::TCpuUsage)aSubCategory)
			{
		CASE_CAT_NAME(EIrqStart);
		CASE_CAT_NAME(EIrqEnd);
		CASE_CAT_NAME(EFiqStart);
		CASE_CAT_NAME(EFiqEnd);
		CASE_CAT_NAME(EIDFCStart);
		CASE_CAT_NAME(EIDFCEnd);
		CASE_CAT_NAME(ENewThreadContext);
			}
		break;

	case BTrace::EChunks:
		switch((BTrace::TChunks)aSubCategory)
			{
		CASE_CAT_NAME(EChunkCreated);
		CASE_CAT_NAME(EChunkInfo);
		CASE_CAT_NAME(EChunkDestroyed);
		CASE_CAT_NAME(EChunkMemoryAllocated);
		CASE_CAT_NAME(EChunkMemoryDeallocated);
		CASE_CAT_NAME(EChunkMemoryAdded);
		CASE_CAT_NAME(EChunkMemoryRemoved);
		CASE_CAT_NAME(EChunkOwner);
			}
		break;

	case BTrace::ECodeSegs:
		switch((BTrace::TCodeSegs)aSubCategory)
			{
		CASE_CAT_NAME(ECodeSegCreated);
		CASE_CAT_NAME(ECodeSegInfo);
		CASE_CAT_NAME(ECodeSegDestroyed);
		CASE_CAT_NAME(ECodeSegMapped);
		CASE_CAT_NAME(ECodeSegUnmapped);
		CASE_CAT_NAME(ECodeSegMemoryAllocated);
		CASE_CAT_NAME(ECodeSegMemoryDeallocated);
			}
		break;
		
	case BTrace::EPaging:
		switch((BTrace::TPaging)aSubCategory)
			{
		CASE_CAT_NAME(EPagingPageInBegin);
		CASE_CAT_NAME(EPagingPageInUnneeded);
		CASE_CAT_NAME(EPagingPageInROM);
		CASE_CAT_NAME(EPagingPageOutROM);
		CASE_CAT_NAME(EPagingPageInFree);
		CASE_CAT_NAME(EPagingPageOutFree);
		CASE_CAT_NAME(EPagingRejuvenate);
		CASE_CAT_NAME(EPagingPageNop);
		CASE_CAT_NAME(EPagingPageLock);
		CASE_CAT_NAME(EPagingPageUnlock);
		CASE_CAT_NAME(EPagingPageOutCache);
		CASE_CAT_NAME(EPagingPageInCode);
		CASE_CAT_NAME(EPagingPageOutCode);
		CASE_CAT_NAME(EPagingMapCode);
		CASE_CAT_NAME(EPagingAged);
		CASE_CAT_NAME(EPagingDecompressStart);
		CASE_CAT_NAME(EPagingDecompressEnd);
		CASE_CAT_NAME(EPagingMemoryModel);
		CASE_CAT_NAME(EPagingChunkDonatePage);
		CASE_CAT_NAME(EPagingChunkReclaimPage);
		CASE_CAT_NAME(EPagingPageIn);
		CASE_CAT_NAME(EPagingPageOut);
		CASE_CAT_NAME(EPagingMapPage);
		CASE_CAT_NAME(EPagingDonatePage);
		CASE_CAT_NAME(EPagingReclaimPage);
		CASE_CAT_NAME(EPagingAgedClean);
		CASE_CAT_NAME(EPagingAgedDirty);
		CASE_CAT_NAME(EPagingPageTableAlloc);
			}
		break;

	case BTrace::EKernelMemory:
		switch((BTrace::TKernelMemory)aSubCategory)
			{
		CASE_CAT_NAME(EKernelMemoryInitialFree);
		CASE_CAT_NAME(EKernelMemoryCurrentFree);
		CASE_CAT_NAME(EKernelMemoryMiscAlloc);
		CASE_CAT_NAME(EKernelMemoryMiscFree);
		CASE_CAT_NAME(EKernelMemoryDemandPagingCache);
		CASE_CAT_NAME(EKernelMemoryDrvPhysAlloc);
		CASE_CAT_NAME(EKernelMemoryDrvPhysFree);
			}
		break;

	case BTrace::EMetaTrace:
		{
		if(aSubCategory==KJunkTraceSubcategory)
			return "*JUNK*";
		else
			{
			switch((BTrace::TMetaTrace)aSubCategory)
				{
			CASE_CAT_NAME(EMetaTraceTimestampsInfo);
			CASE_CAT_NAME(EMetaTraceMeasurementStart);
			CASE_CAT_NAME(EMetaTraceMeasurementEnd);
			CASE_CAT_NAME(EMetaTraceFilterChange);
				}
			}
		}
		break;

	case BTrace::EFastMutex:
		switch((BTrace::TFastMutex)aSubCategory)
			{
		CASE_CAT_NAME(EFastMutexWait);
		CASE_CAT_NAME(EFastMutexSignal);
		CASE_CAT_NAME(EFastMutexFlash);
		CASE_CAT_NAME(EFastMutexName);
		CASE_CAT_NAME(EFastMutexBlock);
			}
		break;

	case BTrace::EProfiling:
		switch((BTrace::TProfiling)aSubCategory)
			{
		CASE_CAT_NAME(ECpuFullSample);
		CASE_CAT_NAME(ECpuOptimisedSample);
		CASE_CAT_NAME(ECpuIdfcSample);
		CASE_CAT_NAME(ECpuNonSymbianThreadSample);
			}
		break;

	case BTrace::ESymbianKernelSync:
		switch((BTrace::TSymbianKernelSync)aSubCategory)
			{
		CASE_CAT_NAME(ESemaphoreCreate);
		CASE_CAT_NAME(ESemaphoreDestroy);
		CASE_CAT_NAME(ESemaphoreAcquire);
		CASE_CAT_NAME(ESemaphoreRelease);
		CASE_CAT_NAME(ESemaphoreBlock);
		CASE_CAT_NAME(EMutexCreate);
		CASE_CAT_NAME(EMutexDestroy);
		CASE_CAT_NAME(EMutexAcquire);
		CASE_CAT_NAME(EMutexRelease);
		CASE_CAT_NAME(EMutexBlock);
		CASE_CAT_NAME(ECondVarCreate);
		CASE_CAT_NAME(ECondVarDestroy);
		CASE_CAT_NAME(ECondVarBlock);
		CASE_CAT_NAME(ECondVarWakeUp);
		CASE_CAT_NAME(ECondVarSignal);
		CASE_CAT_NAME(ECondVarBroadcast);
			}
		break;

	case BTrace::EFlexibleMemModel:
		switch((BTrace::TFlexibleMemModel)aSubCategory)
			{
		CASE_CAT_NAME(EMemoryObjectCreate);
		CASE_CAT_NAME(EMemoryObjectDestroy);
		CASE_CAT_NAME(EMemoryMappingCreate);
		CASE_CAT_NAME(EMemoryMappingDestroy);
		CASE_CAT_NAME(EMemoryObjectIsChunk);
		CASE_CAT_NAME(EMemoryObjectIsCodeSeg);
		CASE_CAT_NAME(EMemoryObjectIsProcessStaticData);
		CASE_CAT_NAME(EMemoryObjectIsDllStaticData);
		CASE_CAT_NAME(EMemoryObjectIsSupervisorStack);
		CASE_CAT_NAME(EMemoryObjectIsUserStack);
		CASE_CAT_NAME(EAddressSpaceId);
			}
		break;

	case BTrace::EHSched:
		switch((BTrace::THSched)aSubCategory)
			{
		CASE_CAT_NAME(ELbDone);
			}
		break;
		}
	return UnknownNames[aSubCategory];
	}



//
// Data structures
//

enum TDataType
	{
	EDataTypeNone = 0,
	EDataTypeText,
	EDataTypeObject,
	};

class Thread;
class Cpu;

struct TraceHeader
	{
	TUint8	iCpuNum;
	TUint8	iCategory;
	TUint8	iSubCategory;
	TUint8	iFlags;
	TUint32	iHeader2;
	TUint64	iTimestamp;
	TUint32	iTimestamp2;
	Thread*	iContextID;
	TUint32	iPC;
	TUint32	iExtra;
	TUint8	iDataTypes[4];
	TUint32	iCalculatedData[2];
	TUint	iDataSize;
	Cpu*	iCpu;
	TUint32	iError;
	};


struct TraceRecord : public TraceHeader
	{
	TUint32 iData[(8+KMaxBTraceDataArray)/4];
	};



//
// ECpuUsage traces
//

enum TContext
	{
	EContextThread,
	EContextFiq,
	EContextIrq,
	EContextIDFC,
	EContextUnknown
	};

class Cpu
	{
public:
	Cpu();
	void ChangeContext(TContext aType, Thread* aThread=0);
	void Reset();

	TContext	iCurrentContext;
	TInt		iFiqNest;
	TInt		iIrqNest;
	TInt		iIDFCNest;
	TUint64		iFiqTime;
	TUint64		iIrqTime;
	TUint64		iIDFCTime;
	Thread*		iCurrentThread;
	TUint64		iBaseTimestamp;
	};

//
// Objects
//

const TUint KMaxTraceNameLength = 10;

class Object
	{
public:
	Object(TUint32 aTraceId, const char* aTraceNamePrefix)
		: iTraceId(aTraceId), iIndex(~0u), iOwner(0), iOwnerTraceId(0), iAlive(1), iNameSet(false), iNameLength(0),
		  iTraceNamePrefix(aTraceNamePrefix)
		{
		iName[0] = 0;
		}

	void Destroy()
		{
		if(iAlive)
			--iAlive;
		if(iOwnerTraceId && !iOwner)
			--UnknownOwners;
		}

	virtual ~Object()
		{}

	void SetName(void* aName, TUint aLength)
		{
		ASSERT(aLength<sizeof(iName));
		iNameLength = (TUint8)aLength;
		memcpy(iName,aName,aLength);
		iName[aLength] = 0;
		iNameSet = true;
		}

	void SetName(TraceRecord& aTrace, TUint aIndex)
		{
		SetName(aTrace.iData+aIndex,aTrace.iDataSize-aIndex*4);
		aTrace.iDataTypes[aIndex] = EDataTypeText;
		}

	TBool IsName(void* aName, TUint aLength)
		{
		if(aLength!=iNameLength)
			return false;
		while(aLength--)
			if(iName[aLength]!=((TUint8*)aName)[aLength])
				return false;
		return true;
		}

	typedef TUint8 TraceNameBuf[KMaxTraceNameLength+1];
	typedef TUint8 FullNameBuf[KMaxBTraceDataArray+2+KMaxBTraceDataArray+2+KMaxBTraceDataArray+1]; // space for name1::name2::name3[tracename]
	typedef TUint8 FullTraceNameBuf[KMaxBTraceDataArray+1+KMaxBTraceDataArray+2+KMaxBTraceDataArray+KMaxTraceNameLength+1+1]; // space for [tracename]'name1::name2::name3'

	TUint FullName(FullNameBuf& aName)
		{
		TUint length = 0;
		if(iOwner)
			{
			if(iOwner->iOwner)
				{
				memcpy(aName+length,iOwner->iOwner->iName,iOwner->iOwner->iNameLength);
				length += iOwner->iOwner->iNameLength;
				aName[length++] = ':';
				aName[length++] = ':';
				}
			memcpy(aName+length,iOwner->iName,iOwner->iNameLength);
			length += iOwner->iNameLength;
			aName[length++] = ':';
			aName[length++] = ':';
			}
		memcpy(aName+length,iName,iNameLength);
		length += iNameLength;
		aName[length] = 0;
		return length;
		}

	TUint TraceName(TraceNameBuf& aName)
		{
		TInt i = 0;
		const TUint KNumDigits = KMaxTraceNameLength-4;
		aName[i++] = '<';
		const char* prefix = iTraceNamePrefix;
		if(prefix[0])
			aName[i++] = *prefix++;
		if(prefix[0])
			aName[i++] = *prefix++;
		TUint n = iIndex;
		for(TUint d=KNumDigits; d>0; --d)
			{
			aName[i+d-1] = TUint8('0'+(n%10));
			n /= 10;
			}
		i += KNumDigits;
		aName[i++] = '>';
		aName[i] = 0;
		return i;
		}

	TUint FullTraceName(FullTraceNameBuf& aName)
		{
		TUint l1 = TraceName(*(TraceNameBuf*)aName);
		aName[l1++] = '\'';
		TUint l2 = FullName(*(FullNameBuf*)(aName+l1));
		aName[l1+l2++] = '\'';
		aName[l1+l2] = 0;
		return l1+l2;
		}

public:
	TUint32 iTraceId;					///< ID for object as found in raw trace data.
	TUint iIndex;						///< Index into container for this object.
	Object* iOwner;						///< Object which 'owns' this one, e.g. process which owns a thread.
	TUint32 iOwnerTraceId;				///< Trace ID for owner if owner object as yet unknown
	TUint8 iAlive;						///< True if object destroyed trace not yet parsed.
	TUint8 iNameSet;					///< True if name has been set.
	TUint8 iNameLength;				
	TUint8 iName[KMaxBTraceDataArray+1];	
	const char* iTraceNamePrefix;
public:
	static TUint32 UnknownOwners;
	};
TUint32 Object::UnknownOwners = 0;


class ObjectContainer
	{
public:
	ObjectContainer()
		: iNumEntries(0), iEntriesLength(0) , iEntries(0)
		{
		iLink = iAllContainers;
		iAllContainers = this;
		}

	static void Reset()
		{
		ObjectContainer* container = iAllContainers;
		while(container)
			{
			TUint i = container->iNumEntries;
			while(i--)
				free(container->iEntries[i].iItem);
			free(container->iEntries);
			container->iEntries = 0;
			container->iNumEntries = 0;
			container->iEntriesLength = 0;
			container = container->iLink;
			}
		}

	void Add(Object* aObject)
		{
		if(iNumEntries>=iEntriesLength)
			{
			iEntriesLength += 128;
			iEntries = (Entry*)realloc(iEntries,iEntriesLength*sizeof(Entry));
			ASSERT(iEntries);
			}
		aObject->iIndex = iNumEntries;
		Entry& entry = iEntries[iNumEntries++];
		entry.iTraceId = aObject->iTraceId;
		entry.iItem = aObject;
		}

	TUint Count()
		{
		return iNumEntries;
		}

	Object* operator[](TInt aIndex)
		{
		if(TUint(aIndex)<iNumEntries)
			return iEntries[aIndex].iItem;
		ASSERT(0);
		return 0;
		}

	Object* Find(TUint32 aTraceId)
		{
		Entry* ptr = iEntries+iNumEntries;
		Entry* end = iEntries;
		while(ptr>end)
			{
			--ptr;
			if(ptr->iTraceId==aTraceId)
				{
				if(ptr->iItem->iAlive)
					return ptr->iItem;
				else
					break;
				}
			}
		return 0;
		}
private:
	struct Entry
		{
		TUint32 iTraceId;
		Object* iItem;
		};
	TUint iNumEntries;
	TUint iEntriesLength;
	Entry* iEntries;
	ObjectContainer* iLink;

	static ObjectContainer* iAllContainers;
	};
ObjectContainer* ObjectContainer::iAllContainers = 0;


#define GENERIC_OBJECT_DEFINITIONS(C)							\
																\
	static C* Find(TUint32 aTraceId)							\
		{														\
		return (C*)iContainer.Find(aTraceId);					\
		}														\
																\
	static C* Create(TraceRecord& aTrace, TUint aIndex)			\
		{														\
		TUint32 traceId = aTrace.iData[aIndex];					\
		C* object = new C(traceId);								\
		aTrace.iDataTypes[aIndex] = EDataTypeObject;			\
		aTrace.iData[aIndex] = (uintptr_t)object;				\
		return object;											\
		}														\
																\
	static C* Find(TraceRecord& aTrace, TUint aIndex)			\
		{														\
		TUint32 traceId = aTrace.iData[aIndex];					\
		C* object = Find(traceId);								\
		if(!object)												\
			return 0;											\
		aTrace.iDataTypes[aIndex] = EDataTypeObject;			\
		aTrace.iData[aIndex] = (uintptr_t)object;				\
		return object;											\
		}														\
																\
	static C* FindOrCreate(TraceRecord& aTrace, TUint aIndex)	\
		{														\
		C* object = Find(aTrace,aIndex);						\
		if(!object)												\
			object = Create(aTrace,aIndex);						\
		return object;											\
		}



//
// Process
//

class Process : public Object
	{
public:
	Process(TUint32 aTraceId)
		: Object(aTraceId,"P"), iThreadCount(0), iMaxThreadCount(0)
		{
		iContainer.Add(this);
		}

	GENERIC_OBJECT_DEFINITIONS(Process);

public:
	TUint iThreadCount;
	TUint iMaxThreadCount;

	static ObjectContainer iContainer;
	};
ObjectContainer Process::iContainer;



//
// Thread
//
class FastMutex;
class Thread : public Object
	{
public:
	Thread(TUint32 aTraceId)
		: Object(aTraceId,"T"), iId(~0u), iCpuTime(0), iSamples(0)
		{
		iContainer.Add(this);
		iNameLength = (TUint8)MakeName(iName,"NThread-",aTraceId);
		}

	TUint64 CpuTime()
		{
		if(!iLastCpu || !iLastCpu->iBaseTimestamp)
			return 0;
		if(iLastCpu->iCurrentThread==this)
			return iCpuTime + Timestamp - iLastCpu->iBaseTimestamp;
		return iCpuTime;
		}

	void Sampled()
		{
		if( iSamples+1 != 0xFFFFFFFF)
			{
			iSamples++;
			}
		}

	GENERIC_OBJECT_DEFINITIONS(Thread);

	static Object* FindThreadOrProcess(TraceRecord& aTrace, TUint aIndex)
		{
		if (!aTrace.iData[aIndex])
			return 0;
		Object* p = Find(aTrace, aIndex);
		if (!p)
			p = Process::Find(aTrace, aIndex);
		return p;
		}
public:
	TUint32 iId;
	Cpu* iLastCpu;
	TUint64 iCpuTime;
	TUint64 iFMBlockStartTime;
	FastMutex* iWaitFastMutex;

	// Number of profiling samples
	TUint32 iSamples;

	static ObjectContainer iContainer;
	};

ObjectContainer Thread::iContainer;



//
// Chunk
//

TUint ChunkErrors = 0;

const TUint KPageShift = 12; // chunk memory is allocated in 4kB pages

class Chunk : public Object
	{
public:
	Chunk(TUint32 aTraceId)
		: Object(aTraceId,"C"), iCurrentSize(0), iPeakSize(0), iMaxSize(0), iPageMap(0), iTraceErrors(0)
		{
		iContainer.Add(this);
		}

	~Chunk()
		{
		free(iPageMap);
		}

	void SetMaxSize(TUint32 aMaxSize)
		{
		ASSERT(!iMaxSize);
		iMaxSize = aMaxSize;
		TUint mapSize = ((aMaxSize>>KPageShift)+7)>>3;
		iPageMap = (TUint8*)malloc(mapSize);
		ASSERT(iPageMap);
		memset(iPageMap,0,mapSize);
		iCurrentSize = 0;
		}

	void Commit(TUint32 aStart,TUint32 aSize)
		{
		if(!iPageMap) // we havent been intialised yet
			return;

		if(aStart+aSize<aStart || aStart+aSize>iMaxSize)
			{
			++iTraceErrors;
			++ChunkErrors;
			return;
			}
		if(SetBits(iPageMap,aStart>>KPageShift,aSize>>KPageShift))
			{
			++iTraceErrors;
			++ChunkErrors;
			ErrorOnThisTrace = true;
			}
		}

	void Decommit(TUint32 aStart,TUint32 aSize)
		{
		if(!iPageMap) // we havent been intialised yet
			return;

		// Decommit is complicated by the fact that aSize is the number of pages
		// actually decommited, which may be less than the region of the original
		// chunk decommit operation. E.g. if pages 1 and 3 were originally decommited
		// and the decommit operation was for pages 0-3, then the trace has a size of
		// 2 pages, even though the operation was on 4 pages.
		// We handle this, repeatedly decommiting from our iPageMap, until we have
		// freed aSize bytes worth of pages...
		while(aSize)
			{
			if(aStart+aSize<aStart || aStart+aSize>iMaxSize)
				{
				// we haven't found enough memory to decommit
				++iTraceErrors;
				++ChunkErrors;
				ErrorOnThisTrace = true;
				return;
				}
			TUint notDecommitted = ClearBits(iPageMap,aStart>>KPageShift,aSize>>KPageShift);
			aStart += aSize;
			aSize = notDecommitted<<KPageShift;
			}
		}

	void ResetMemory()
		{
		}

	GENERIC_OBJECT_DEFINITIONS(Chunk);

public:
	TUint32 iCurrentSize;
	TUint32 iPeakSize;
	TUint32 iMaxSize;
	TUint8* iPageMap;
	TUint iTraceErrors;

	static ObjectContainer iContainer;
	};
ObjectContainer Chunk::iContainer;



//
// Semaphore, Mutex, CondVar
//
class Semaphore : public Object
	{
public:
	Semaphore(TUint32 aTraceId)
		: Object(aTraceId,"S")
		{
		iContainer.Add(this);
		}

	~Semaphore()
		{
		}

	GENERIC_OBJECT_DEFINITIONS(Semaphore);
public:

	static ObjectContainer iContainer;
	};
ObjectContainer Semaphore::iContainer;


class Mutex : public Object
	{
public:
	Mutex(TUint32 aTraceId)
		: Object(aTraceId,"M")
		{
		iContainer.Add(this);
		}

	~Mutex()
		{
		}

	GENERIC_OBJECT_DEFINITIONS(Mutex);
public:

	static ObjectContainer iContainer;
	};
ObjectContainer Mutex::iContainer;


class CondVar : public Object
	{
public:
	CondVar(TUint32 aTraceId)
		: Object(aTraceId,"V")
		{
		iContainer.Add(this);
		}

	~CondVar()
		{
		}

	GENERIC_OBJECT_DEFINITIONS(CondVar);
public:

	static ObjectContainer iContainer;
	};
ObjectContainer CondVar::iContainer;




//
// CodeSeg
//

TUint CodeSegErrors = 0;

class CodeSeg : public Object
	{
public:
	CodeSeg(TUint32 aTraceId)
		: Object(aTraceId,"CS"), iAllocatedMemory(0)
		{
		iContainer.Add(this);
		}

	GENERIC_OBJECT_DEFINITIONS(CodeSeg);

	TUint iAllocatedMemory;
public:
	static ObjectContainer iContainer;
	};
ObjectContainer CodeSeg::iContainer;



//
// FastMutex
//

TUint FastMutexNestErrors = 0;

class FastMutex : public Object
	{
public:
	FastMutex(TUint32 aTraceId)
		: Object(aTraceId,"FM"), iHoldingThread(0), iHeldCount(0), iTotalHeldTime(0),
		  iMaxHeldTime(0), iMaxHeldTimestamp(0), iMaxHeldPc(0), iBlockCount(0)
		{
		iContainer.Add(this);
		iNameLength = (TUint8)MakeName(iName,"NFastMutex-",aTraceId);
		iNameLength = 19;
		}

	TUint32 Wait(Thread* aThread)
		{
		TUint32 time = 0;
		if(iHoldingThread)
			{
			++FastMutexNestErrors;
			ErrorOnThisTrace = true;
			}
		iHoldingThread = aThread;
		iWaitCpuTimeBase = aThread->CpuTime();
		if (aThread->iWaitFastMutex == this)
			{
			time = (TUint32)(Timestamp - aThread->iFMBlockStartTime);
			}
		aThread->iWaitFastMutex = 0;
		return time;
		}

	void Block(Thread* aThread)
		{
		if (aThread->iWaitFastMutex != this)
			{
			aThread->iFMBlockStartTime = Timestamp;
			aThread->iWaitFastMutex = this;
			++iBlockCount;
			}
		};

	TUint32 Signal(Thread* aThread, TUint32 aPc)
		{
		if (!iHoldingThread)	// First record for this mutex so ignore it as don't
			return 0;			// have waiting thread details
		
		if(iHoldingThread!=aThread)
			{
			++FastMutexNestErrors;
			ErrorOnThisTrace = true;
			iHoldingThread = 0;
			return 0;
			}
		iHoldingThread = 0;
		TUint64 time = aThread->CpuTime()-iWaitCpuTimeBase;
		++iHeldCount;
		iTotalHeldTime += time;
		if(time>iMaxHeldTime)
			{
			iMaxHeldTime = time;
			iMaxHeldPc = aPc;
			iMaxHeldTimestamp = Timestamp;
			}
		return (TUint32)time;
		}

	GENERIC_OBJECT_DEFINITIONS(FastMutex);

public:
	Thread* iHoldingThread;
	TUint32 iHeldCount;		// number of times mutex acquired
	TUint64 iTotalHeldTime;
	TUint64 iWaitCpuTimeBase;
	TUint64 iMaxHeldTime;
	TUint64 iMaxHeldTimestamp;
	TUint32 iMaxHeldPc;
	TUint32 iBlockCount;	// number of times mutex caused a thread to wait

	static ObjectContainer iContainer;
	};
ObjectContainer FastMutex::iContainer;




//
// ProfilingSample
//

TUint ProfilingSampleErrors = 0;

class ProfilingSample : public Object
	{
public:
	ProfilingSample(TUint32 aTraceId)
		: Object(aTraceId,"PS")
		{
		iContainer.Add(this);
		iNameLength = (TUint8)MakeName(iName,"ProfilingSample-",aTraceId);
		}

	void SetPC( TUint32 aPC )
		{
		iPC = aPC;
		}

	void SetThread( TUint32 aThread )
		{
		if( 0 != aThread )
			iThread = aThread;
		}

	void SetType(const BTrace::TProfiling aType)
		{
		iType = aType;
		}

	GENERIC_OBJECT_DEFINITIONS(ProfilingSample);


public:

	static ObjectContainer iContainer;
	static Thread* iTopTen[10];
	static TUint32 iSamples;
	static TUint32 iLastThread;

	TUint32 iPC;
	TUint32 iThread;
	BTrace::TProfiling iType;

	};

TUint32 ProfilingSample::iSamples = 0;
TUint32 ProfilingSample::iLastThread = 0;

ObjectContainer ProfilingSample::iContainer;


//
// EThreadIdentification traces
//

void PreProcessThreadIdentification(TraceRecord& aTrace)
	{
	Thread* thread;
	Process* process;

	switch((BTrace::TThreadIdentification)aTrace.iSubCategory)
		{
	case BTrace::ENanoThreadCreate:
		CHECK_TRACE_DATA_WORDS(1);
		Thread::FindOrCreate(aTrace,0);
		break;

	case BTrace::ENanoThreadDestroy:
		CHECK_TRACE_DATA_WORDS(1);
		thread = Thread::Find(aTrace,0);
		if(thread)
			thread->Destroy();
		break;

	case BTrace::EThreadCreate:
	case BTrace::EThreadName:
		CHECK_TRACE_DATA_WORDS(2);
		thread = Thread::FindOrCreate(aTrace,0);
		if(aTrace.iSubCategory==BTrace::EThreadCreate)
			++thread->iAlive; // thread needs destroying twice (ENanoThreadDestroy+EThreadDestroy)
		process = Process::FindOrCreate(aTrace,1);
		thread->iOwner = process;
		++process->iThreadCount;
		if(process->iThreadCount>process->iMaxThreadCount)
			process->iMaxThreadCount = process->iThreadCount;
		thread->SetName(aTrace,2);
		break;

	case BTrace::EThreadDestroy:
		CHECK_TRACE_DATA_WORDS(1);
		thread = Thread::Find(aTrace,0);
		if(thread)
			{
			thread->Destroy();
			process = (Process*)thread->iOwner;
			if(process && process->iThreadCount)
				--process->iThreadCount;
			}
		break;

	case BTrace::EProcessName:
		CHECK_TRACE_DATA_WORDS(2);
		if(aTrace.iData[0])
			{
			Thread::FindOrCreate(aTrace,0);
			process = Process::Find(aTrace.iData[1]);
			if(!process || (process->iNameLength && !process->IsName(aTrace.iData+2,aTrace.iDataSize-2*4)))
				{
				if(process)
					process->Destroy();
				process = Process::Create(aTrace,1); // no existing process, or name different
				}
			else
				process = Process::Find(aTrace,1); // find again (this will update trace data[1])
			}
		else
			process = Process::Find(aTrace,1);
		if(process)
			process->SetName(aTrace,2);
		break;

	case BTrace::EThreadId:
		CHECK_TRACE_DATA_WORDS(2);
		thread = Thread::FindOrCreate(aTrace,0);
		Process::FindOrCreate(aTrace,1);
		thread->iId = aTrace.iData[2];
		break;

	case BTrace::EProcessCreate:
		CHECK_TRACE_DATA_WORDS(1);
		Process::FindOrCreate(aTrace,0);
		break;

	case BTrace::EProcessDestroy:
		CHECK_TRACE_DATA_WORDS(1);
		process = Process::FindOrCreate(aTrace,0);
		if(process)
			process->Destroy();
		break;

	default:
		break;
		}
	}


//
// ECpuUsage traces
//

Cpu TheCpus[KMaxCpus];
TUint InterruptNestErrors = 0;
TUint CpuUsagePresent = 0;

Cpu::Cpu()
	:	iCurrentContext(EContextUnknown),
		iFiqNest(0),
		iIrqNest(0),
		iIDFCNest(0),
		iFiqTime(0),
		iIrqTime(0),
		iIDFCTime(0),
		iCurrentThread(0),
		iBaseTimestamp(0)
	{
	}

void Cpu::Reset()
	{
	iCurrentContext = EContextUnknown;
	iFiqNest = 0;
	iIrqNest = 0;
	iIDFCNest = 0;
	iCurrentThread = 0;
	iBaseTimestamp = 0;
	}

void ResetCpuUsage()
	{
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		TheCpus[i].Reset();
	}


void StartCpuUsage()
	{
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		new (&TheCpus[i]) Cpu;
	InterruptNestErrors = 0;
	}


void Cpu::ChangeContext(TContext aType, Thread* aThread)
	{
	TUint64 delta = Timestamp-iBaseTimestamp;
	switch(iCurrentContext)
		{
	case EContextThread:
		iCurrentThread->iCpuTime += delta;
		break;
	case EContextFiq:
		iFiqTime += delta;
		break;
	case EContextIrq:
		iIrqTime += delta;
		break;
	case EContextIDFC:
		iIDFCTime += delta;
		break;
	default:
		break;
		}

	if(aType==EContextUnknown)
		{
		if(iFiqNest)
			aType = EContextFiq;
		else if(iIrqNest)
			aType = EContextIrq;
		else if(iIDFCNest)
			aType = EContextIDFC;
		else
			{
			aType = EContextThread;
			aThread = iCurrentThread;
			}
		}

	if(aType==EContextThread && !aThread)
		{
		iCurrentContext = EContextUnknown;
		iBaseTimestamp = 0;
		return;
		}

	iCurrentContext = aType;
	if(aType==EContextThread)
		{
		iCurrentThread = aThread;
		aThread->iLastCpu = this;
		}

	iBaseTimestamp = Timestamp;
	}


void PreProcessCpuUsage(TraceRecord& aTrace)
	{
	CpuUsagePresent = true;
	Cpu& cpu = *aTrace.iCpu;

	switch((BTrace::TCpuUsage)aTrace.iSubCategory)
		{
	case BTrace::EIrqStart:
		++cpu.iIrqNest;
		cpu.ChangeContext(EContextIrq);
		break;

	case BTrace::EFiqStart:
		++cpu.iFiqNest;
		cpu.ChangeContext(EContextFiq);
		break;

	case BTrace::EIDFCStart:
		if(cpu.iIrqNest+cpu.iFiqNest>1 || cpu.iIDFCNest!=0)
			{
			++InterruptNestErrors;
			ErrorOnThisTrace = true;
			}
		cpu.iIrqNest = 0;
		cpu.iFiqNest = 0;
		cpu.iIDFCNest = 1;
		cpu.ChangeContext(EContextIDFC);
		break;

	case BTrace::EIrqEnd:
		if(cpu.iIrqNest)
			--cpu.iIrqNest;
		else
			{
			++InterruptNestErrors;
			ErrorOnThisTrace = true;
			}
		cpu.ChangeContext(EContextUnknown);
		break;

	case BTrace::EFiqEnd:
		if(cpu.iFiqNest)
			--cpu.iFiqNest;
		else
			{
			++InterruptNestErrors;
			ErrorOnThisTrace = true;
			}
		cpu.ChangeContext(EContextUnknown);
		break;

	case BTrace::EIDFCEnd:
		if(cpu.iIDFCNest!=1)
			{
			++InterruptNestErrors;
			ErrorOnThisTrace = true;
			}
		cpu.iIDFCNest = 0;
		cpu.ChangeContext(EContextUnknown);
		break;

	case BTrace::ENewThreadContext:
		if(cpu.iIrqNest+cpu.iFiqNest>1 || cpu.iIDFCNest!=0)
			{
			++InterruptNestErrors;
			ErrorOnThisTrace = true;
			}
		cpu.iIrqNest = 0;
		cpu.iFiqNest = 0;
		cpu.iIDFCNest = 0;
		cpu.ChangeContext(EContextThread,aTrace.iContextID);
		break;
		}
	}


void ReportThreads()
	{
	TUint numThreads = Thread::iContainer.Count();
	if(!numThreads)
		return;

	TUint64 totalTime = 0;
	printf("\nREPORT: Threads\n\n");
	WarnIfError(0);
	printf("%-10s %5s %10s %8s %s\n","","State","CPUTime","TraceId","Name");
	TUint i;
	for(i=0; i<numThreads; ++i)
		{
		Thread* thread = (Thread*)Thread::iContainer[i];
		Object::FullNameBuf fullName;
		thread->FullName(fullName);
		Object::TraceNameBuf name;
		thread->TraceName(name);
		printf("%-10s %5s %10d %08x '%s'\n",name,
			thread->iAlive?(const char*)"Alive":(const char*)"Dead",
			(int)Time(thread->iCpuTime),(int)thread->iTraceId,fullName);
		totalTime += thread->iCpuTime;
		}
	for (i=0; i<(TUint)KMaxCpus; ++i)
		{
		printf("CPU %1d\n", i);
		Cpu& cpu = TheCpus[i];
		printf("%-10s %5s %10d %s\n","FIQ","",(int)Time(cpu.iFiqTime),"");
		printf("%-10s %5s %10d %s\n","IRQ","",(int)Time(cpu.iIrqTime),"");
		printf("%-10s %5s %10d %s\n","IDFC","",(int)Time(cpu.iIDFCTime),"");
		totalTime += cpu.iFiqTime + cpu.iIrqTime + cpu.iIDFCTime;
		}
	printf("%-10s %5s ----------\n","","");
	printf("%-10s %5s %10d\n","","",(int)Time(totalTime));
	printf("\n");
	}


void ReportProcesses()
	{
	TUint numProcesses = Process::iContainer.Count();
	if(!numProcesses)
		return;

	printf("\nREPORT: Processes\n\n");
	WarnIfError(0);
	printf("%-10s %5s %7s %8s %s\n","","State","Threads","TraceId","Name");
	TUint i;
	for(i=0; i<numProcesses; ++i)
		{
		Process* process = (Process*)Process::iContainer[i];
		Object::FullNameBuf fullName;
		process->FullName(fullName);
		Object::TraceNameBuf name;
		process->TraceName(name);
		printf("%-10s %5s %3u/%-3u %08x '%s'\n",name,
			process->iAlive?(const char*)"Alive":(const char*)"Dead",
			(unsigned int)process->iThreadCount,(unsigned int)process->iMaxThreadCount,
			(unsigned int)process->iTraceId,fullName);
		}
	printf("\n");
	}


void EndCpuUsage()
	{
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		TheCpus[i].ChangeContext(EContextUnknown);
	}


//
// EChunks traces
//

void StartChunks()
	{
	ChunkErrors = 0;
	}


void PreProcessChunks(TraceRecord& aTrace)
	{
	CHECK_TRACE_DATA_WORDS(1);
	Chunk* chunk = Chunk::FindOrCreate(aTrace,0);

	switch((BTrace::TChunks)aTrace.iSubCategory)
		{
	case BTrace::EChunkCreated:
		CHECK_TRACE_DATA_WORDS(2);
		chunk->SetName(aTrace,2);
		chunk->SetMaxSize(aTrace.iData[1]);
		// start by assuming thread is 'owned' by the thread which created it...
		chunk->iOwner = aTrace.iContextID;
		break;

	case BTrace::EChunkInfo:
		CHECK_TRACE_DATA_WORDS(3);
		break; // ignore

	case BTrace::EChunkDestroyed:
		chunk->Destroy();
		break;

	case BTrace::EChunkMemoryAllocated:
		{
		CHECK_TRACE_DATA_WORDS(3);
		chunk->Commit(aTrace.iData[1],aTrace.iData[2]);
		TUint32 size = chunk->iCurrentSize+aTrace.iData[2];
		if(size<chunk->iCurrentSize || size>chunk->iMaxSize)
			size = chunk->iMaxSize;
		chunk->iCurrentSize = size;
		if(size>chunk->iPeakSize)
			chunk->iPeakSize = size;
		aTrace.iCalculatedData[0] = size/1024;
		}
		break;

	case BTrace::EChunkMemoryDeallocated:
		{
		CHECK_TRACE_DATA_WORDS(3);
		chunk->Decommit(aTrace.iData[1],aTrace.iData[2]);
		TUint32 size = chunk->iCurrentSize-aTrace.iData[2];
		if(size>chunk->iCurrentSize)
			size = 0;
		chunk->iCurrentSize = size;
		aTrace.iCalculatedData[0] = size/1024;
		}
		break;

	case BTrace::EChunkMemoryAdded:
		CHECK_TRACE_DATA_WORDS(3);
		chunk->Commit(aTrace.iData[1],aTrace.iData[2]);
		break;

	case BTrace::EChunkMemoryRemoved:
		CHECK_TRACE_DATA_WORDS(3);
		chunk->Decommit(aTrace.iData[1],aTrace.iData[2]);
		break;

	case BTrace::EChunkOwner:
		{
		CHECK_TRACE_DATA_WORDS(2);
		Process* process = Process::FindOrCreate(aTrace,1);
		// set owner, unless current owner is owned by the same process
		// (this preserves creating thread names in ownership list which is more useful)
		if(!chunk->iOwner || chunk->iOwner->iOwner!=process)
			chunk->iOwner = process;
		}
		break;

		}
	}


void ReportChunks()
	{
	TUint numChunks = Chunk::iContainer.Count();
	if(!numChunks)
		return;

	if(!ReportLevel)
		printf("\nREPORT: Chunks (Named objects only)\n\n");
	else
		printf("\nREPORT: Chunks\n\n");
	WarnIfError(ChunkErrors);
	printf("%-10s %5s %8s %8s %8s %8s %s\n",
		"","State","Size","Peak","Max","TraceId","Name");
	TUint totalSize = 0;
	TUint i;
	for(i=0; i<numChunks; ++i)
		{
		Chunk* chunk = (Chunk*)Chunk::iContainer[i];
		if(ReportLevel==0 && !chunk->iNameSet)
			continue; // only report explicitly named mutexes at report level 0
		Object::FullNameBuf fullName;
		chunk->FullName(fullName);
		Object::TraceNameBuf name;
		chunk->TraceName(name);
		printf("%-10s %5s %7uk %7uk %7uk %08x '%s'\n",
			name,chunk->iAlive?(const char*)"Alive":(const char*)"Dead",
			(unsigned int)chunk->iCurrentSize/1024,(unsigned int)chunk->iPeakSize/1024,(unsigned int)chunk->iMaxSize/1024,
			(unsigned int)chunk->iTraceId,fullName);
		totalSize += chunk->iCurrentSize/1024;
		}
	printf("%-10s %5s --------\n","","");
	printf("%-10s %5s %7uk\n","","",totalSize);
	printf("\n");
	}



//
// CodeSeg
//

void StartCodeSegs()
	{
	CodeSegErrors = 0;
	}


void PreProcessCodeSegs(TraceRecord& aTrace)
	{
	CHECK_TRACE_DATA_WORDS(1);
	CodeSeg* codeseg;

	switch((BTrace::TCodeSegs)aTrace.iSubCategory)
		{
	case BTrace::ECodeSegCreated:
		codeseg = CodeSeg::FindOrCreate(aTrace,0);
		codeseg->SetName(aTrace,1);
		break;

	case BTrace::ECodeSegInfo:
		CHECK_TRACE_DATA_WORDS(10);
		CodeSeg::FindOrCreate(aTrace,0);
/*		- 4 bytes containing the attributes.
		- 4 bytes containing the code base address (.text).
		- 4 bytes containing the size of the code section (.text).
		- 4 bytes containing the base address of the constant data section (.radata).
		- 4 bytes containing the size of the constant data section (.radata).
		- 4 bytes containing the base address of the initialised data section (.data).
		- 4 bytes containing the size of the initialised data section (.data).
		- 4 bytes containing the base address of the uninitialised data section (.bss).
		- 4 bytes containing the size of the uninitialised data section (.bss).
*/		break;

	case BTrace::ECodeSegDestroyed:
		codeseg = CodeSeg::FindOrCreate(aTrace,0);
		codeseg->Destroy();
		codeseg->iAllocatedMemory = 0; // clear this now because ECodeSegMemoryDeallocated comes after codeseg destroy
		break;

	case BTrace::ECodeSegMapped:
		CHECK_TRACE_DATA_WORDS(2);
		CodeSeg::FindOrCreate(aTrace,0);
		Process::FindOrCreate(aTrace,1);
		break;

	case BTrace::ECodeSegUnmapped:
		CHECK_TRACE_DATA_WORDS(2);
		CodeSeg::FindOrCreate(aTrace,0);
		Process::FindOrCreate(aTrace,1);
		break;

	case BTrace::ECodeSegMemoryAllocated:
		CHECK_TRACE_DATA_WORDS(2);
		codeseg = CodeSeg::FindOrCreate(aTrace,0);
		codeseg->iAllocatedMemory += aTrace.iData[1];
		if(codeseg->iAllocatedMemory<aTrace.iData[1])
			{
			codeseg->iAllocatedMemory = ~0u; // overflowed!
			++CodeSegErrors;
			ErrorOnThisTrace = true;
			}
		break;

	case BTrace::ECodeSegMemoryDeallocated:
		{
		CHECK_TRACE_DATA_WORDS(2);
		codeseg = CodeSeg::Find(aTrace,0);
		if(codeseg)
			{
			TUint32 memory = codeseg->iAllocatedMemory-aTrace.iData[1];
			if(memory>codeseg->iAllocatedMemory)
				{
				memory = 0; // underflowed
				++CodeSegErrors;
				ErrorOnThisTrace = true;
				}
			codeseg->iAllocatedMemory = memory;
			}
		}
		break;

		}
	}


void ReportCodeSegs()
	{
	TUint numCodeSegs = CodeSeg::iContainer.Count();
	if(!numCodeSegs)
		return;

	if(!ReportLevel)
		printf("\nREPORT: CodeSegs (Named objects only)\n\n");
	else
		printf("\nREPORT: CodeSegs\n\n");
	WarnIfError(CodeSegErrors);
	printf("%-10s %5s %8s %8s %s\n",
		"","State","Memory","TraceId","Name");
	TUint totalSize = 0;
	TUint i;
	for(i=0; i<numCodeSegs; ++i)
		{
		CodeSeg* codeseg = (CodeSeg*)CodeSeg::iContainer[i];
		if(ReportLevel==0 && !codeseg->iNameSet)
			continue; // only report explicitly named mutexes at report level 0
		Object::FullNameBuf fullName;
		codeseg->FullName(fullName);
		Object::TraceNameBuf name;
		codeseg->TraceName(name);
		printf("%-10s %5s %7uk %08x '%s'\n",
			name,codeseg->iAlive?(const char*)"Alive":(const char*)"Dead",
			(unsigned int)codeseg->iAllocatedMemory/1024,(unsigned int)codeseg->iTraceId,fullName);
		totalSize += codeseg->iAllocatedMemory/1024;
		}
	printf("%-10s %5s --------\n","","");
	printf("%-10s %5s %7uk\n","","",totalSize);
	printf("\n");
	}



//
// MetaTrace
//

TUint KernelMemoryInitialFree = 0;
TUint KernelMemoryCurrentFree = 0;
TUint KernelMemoryMisc = 0;
TUint KernelMemoryDrvPhys = 0;
TUint KernelMemoryDemandPagingCache = 0;
TUint KernelMemoryErrors = 0;
TBool KernelMemoryTracesPresent = false;

void StartKernelMemory()
	{
	KernelMemoryInitialFree = 0;
	KernelMemoryCurrentFree = 0;
	KernelMemoryMisc = 0;
	KernelMemoryDrvPhys = 0;
	KernelMemoryErrors = 0;
	KernelMemoryTracesPresent = false;
	}


void PreProcessKernelMemory(TraceRecord& aTrace)
	{
	CHECK_TRACE_DATA_WORDS(1);
	KernelMemoryTracesPresent = true;
	switch((BTrace::TKernelMemory)aTrace.iSubCategory)
		{
	case BTrace::EKernelMemoryInitialFree:
		KernelMemoryInitialFree = aTrace.iData[0];
		aTrace.iCalculatedData[0] = KernelMemoryInitialFree/1024;
		break;

	case BTrace::EKernelMemoryCurrentFree:
		KernelMemoryCurrentFree = aTrace.iData[0];
		aTrace.iCalculatedData[0] = KernelMemoryCurrentFree/1024;
		break;

	case BTrace::EKernelMemoryMiscAlloc:
		KernelMemoryMisc += aTrace.iData[0];
		if(KernelMemoryMisc < aTrace.iData[0])
			{
			KernelMemoryMisc = 0xffffffffu;
			++KernelMemoryErrors;
			ErrorOnThisTrace = true;
			}
		aTrace.iCalculatedData[0] = KernelMemoryMisc/1024;
		break;

	case BTrace::EKernelMemoryMiscFree:
		if(KernelMemoryMisc >= aTrace.iData[0])
			KernelMemoryMisc -= aTrace.iData[0];
		else
			{
			KernelMemoryMisc = 0;
			++KernelMemoryErrors;
			ErrorOnThisTrace = true;
			}
		aTrace.iCalculatedData[0] = KernelMemoryMisc/1024;
		break;

	case BTrace::EKernelMemoryDemandPagingCache:
		KernelMemoryDemandPagingCache = aTrace.iData[0];
		break;

	case BTrace::EKernelMemoryDrvPhysAlloc:
		KernelMemoryDrvPhys += aTrace.iData[0];
		if(KernelMemoryDrvPhys < aTrace.iData[0])
			{
			KernelMemoryDrvPhys = 0xffffffffu;
			++KernelMemoryErrors;
			ErrorOnThisTrace = true;
			}
		aTrace.iCalculatedData[0] = KernelMemoryDrvPhys/1024;
		break;

	case BTrace::EKernelMemoryDrvPhysFree:
		if(KernelMemoryDrvPhys >= aTrace.iData[0])
			KernelMemoryDrvPhys -= aTrace.iData[0];
		else
			{
			KernelMemoryDrvPhys = 0;
			++KernelMemoryErrors;
			ErrorOnThisTrace = true;
			}
		aTrace.iCalculatedData[0] = KernelMemoryDrvPhys/1024;
		break;
		}
	}

void ReportKernelMemory()
	{
	if(!KernelMemoryTracesPresent)
		return;

	printf("\nREPORT: Kernel Memory\n\n");
	WarnIfError(KernelMemoryErrors);
	printf("Total RAM size............................. %dk\n",KernelMemoryInitialFree/1024);
	printf("Miscelaneous RAM used by kernel............ %dk\n",KernelMemoryMisc/1024);
	printf("Physical RAM allocated by device drivers... %dk\n",KernelMemoryDrvPhys/1024);
	printf("Demand paging cache reserve................ %dk\n",KernelMemoryDemandPagingCache/1024);
	if(ReportLevel>1)
		printf("Last 'current free RAM' value seen......... %dk\n",KernelMemoryCurrentFree/1024);

	printf("\n");
	}

//
// MetaTrace
//

void StartMetaTrace()
	{
	TimestampPeriod = 0;
	Timestamp2Period = 0;
	}


void PreProcessMetaTrace(TraceRecord& aTrace)
	{
	switch((BTrace::TMetaTrace)aTrace.iSubCategory)
		{
	case BTrace::EMetaTraceTimestampsInfo:
		CHECK_TRACE_DATA_WORDS(3);
		TimestampPeriod = aTrace.iData[0];
		Timestamp2Period = aTrace.iData[1];
		Timestamp64Bit = aTrace.iData[2]&1;
		break;

	case BTrace::EMetaTraceMeasurementStart:
	case BTrace::EMetaTraceMeasurementEnd:
		CHECK_TRACE_DATA_WORDS(2);
		aTrace.iDataTypes[2] = EDataTypeText;
		break;

	case BTrace::EMetaTraceFilterChange:
		CHECK_TRACE_DATA_WORDS(1);
		break;
		}
	}

//
// EFastMutex traces
//

void StartFastMutex()
	{
	FastMutexNestErrors = 0;
	}


void PreProcessFastMutex(TraceRecord& aTrace)
	{
	CHECK_TRACE_DATA_WORDS(1);
	FastMutex* mutex = FastMutex::FindOrCreate(aTrace,0);
	Thread* thread = aTrace.iContextID;

	switch((BTrace::TFastMutex)aTrace.iSubCategory)
		{
	case BTrace::EFastMutexWait:
		aTrace.iCalculatedData[0] = Time(mutex->Wait(thread));
		break;

	case BTrace::EFastMutexSignal:
		aTrace.iCalculatedData[0] = Time(mutex->Signal(thread,aTrace.iPC));
		break;

	case BTrace::EFastMutexFlash:
		aTrace.iCalculatedData[0] = Time(mutex->Signal(thread,aTrace.iPC));
		mutex->Wait(thread);
		break;

	case BTrace::EFastMutexName:
		CHECK_TRACE_DATA_WORDS(2);
		mutex->SetName(aTrace,2);
		break;

	case BTrace::EFastMutexBlock:
		mutex->Block(thread);
		break;

		}
	}


void PreProcessSymbianKernelSync(TraceRecord& aTrace)
	{
	switch((BTrace::TSymbianKernelSync)aTrace.iSubCategory)
		{
	case BTrace::ESemaphoreCreate:
		{
		CHECK_TRACE_DATA_WORDS(2);
		TUint32 ownerid = aTrace.iData[1];
		Semaphore* sem = Semaphore::FindOrCreate(aTrace,0);
		Object* owner = Thread::FindThreadOrProcess(aTrace,1);
		sem->iOwner = owner;
		if (!owner && ownerid)
			sem->iOwnerTraceId = ownerid, ++Object::UnknownOwners;
		sem->SetName(aTrace,2);
		break;
		}

	case BTrace::ESemaphoreDestroy:
		{
		CHECK_TRACE_DATA_WORDS(1);
		Semaphore* sem = Semaphore::Find(aTrace,0);
		if (sem)
			sem->Destroy();
		break;
		}

	case BTrace::ESemaphoreAcquire:
	case BTrace::ESemaphoreRelease:
	case BTrace::ESemaphoreBlock:
		{
		CHECK_TRACE_DATA_WORDS(1);
		Semaphore::FindOrCreate(aTrace,0);
		break;
		}


	case BTrace::EMutexCreate:
		{
		CHECK_TRACE_DATA_WORDS(2);
		TUint32 ownerid = aTrace.iData[1];
		Mutex* m = Mutex::FindOrCreate(aTrace,0);
		Object* owner = Thread::FindThreadOrProcess(aTrace,1);
		m->iOwner = owner;
		if (!owner && ownerid)
			m->iOwnerTraceId = ownerid, ++Object::UnknownOwners;
		m->SetName(aTrace,2);
		break;
		}

	case BTrace::EMutexDestroy:
		{
		CHECK_TRACE_DATA_WORDS(1);
		Mutex* m = Mutex::Find(aTrace,0);
		if (m)
			m->Destroy();
		break;
		}

	case BTrace::EMutexAcquire:
	case BTrace::EMutexRelease:
	case BTrace::EMutexBlock:
		{
		CHECK_TRACE_DATA_WORDS(1);
		Mutex::FindOrCreate(aTrace,0);
		break;
		}


	case BTrace::ECondVarCreate:
		{
		CHECK_TRACE_DATA_WORDS(2);
		TUint32 ownerid = aTrace.iData[1];
		CondVar* cv = CondVar::FindOrCreate(aTrace,0);
		Object* owner = Thread::FindThreadOrProcess(aTrace,1);
		cv->iOwner = owner;
		if (!owner && ownerid)
			cv->iOwnerTraceId = ownerid, ++Object::UnknownOwners;
		cv->SetName(aTrace,2);
		break;
		}

	case BTrace::ECondVarDestroy:
		{
		CHECK_TRACE_DATA_WORDS(1);
		CondVar* cv = CondVar::Find(aTrace,0);
		if (cv)
			cv->Destroy();
		break;
		}

	case BTrace::ECondVarBlock:
	case BTrace::ECondVarWakeUp:
	case BTrace::ECondVarSignal:
	case BTrace::ECondVarBroadcast:
		{
		CHECK_TRACE_DATA_WORDS(1);
		CondVar::FindOrCreate(aTrace,0);
		break;
		}


	default:
		break;
		}
	}


void ReportFastMutex()
	{
	TUint numMutexes = FastMutex::iContainer.Count();
	if(!numMutexes)
		return;

	if(!ReportLevel)
		printf("\nREPORT: FastMutexes (Named objects only)\n\n");
	else
		printf("\nREPORT: FastMutexes\n\n");
	WarnIfError(0);
	printf("%-10s %8s %8s %10s %10s %-8s %12s %8s %s\n",
		"","MaxTime","AveTime","HeldCount","BlockCount","MaxPC","MaxTimestamp","TraceId","Name");
	TUint i;
	for(i=0; i<numMutexes; ++i)
		{
		FastMutex* mutex = (FastMutex*)FastMutex::iContainer[i];
		if(ReportLevel==0 && !mutex->iNameSet)
			continue; // only report explicitly named mutexes at report level 0
		Object::FullNameBuf fullName;
		mutex->FullName(fullName);
		Object::TraceNameBuf name;
		mutex->TraceName(name);
		TUint32 averageHeldTime = mutex->iHeldCount ? Time(mutex->iTotalHeldTime/mutex->iHeldCount) : 0;
		printf("%-10s %8u %8u %10u %10u %08x %12u %08x '%s'\n",
			name,(unsigned int)Time(mutex->iMaxHeldTime),(unsigned int)averageHeldTime,(unsigned int)mutex->iHeldCount,(unsigned int)mutex->iBlockCount,
			(unsigned int)mutex->iMaxHeldPc,(unsigned int)Time(mutex->iMaxHeldTimestamp-TimestampBase),(unsigned int)mutex->iTraceId,fullName);
		}
	printf("\n");
	}


//
// ProfilingSample
//

void StartProfilingSample()
	{
	ProfilingSampleErrors = 0;
	}


/** 
	Index 0 of TraceRecord is the program counter
	The only one not having it are ECpuNonSymbianThreadSample samples.
	Index 1 of TraceRecord is the NThread pointer.
	The only one that has it is ECpuFullSample.
	The samples are identified by their index, which is maintained by 
	ProfilingSample::iSamples. Thus to create a ProfilingSample object we
	need to put this value in the data at index 0 after copying the PC 
	and Thread id (if present).
	The reasoning is that all samples need to be represented and thus we 
	need to create a ProfilingSample object, even when they are on the same 
	PC and or thread. Each sample important and should not be discarded.
*/
void PreProcessProfiling(TraceRecord& aTrace)
	{

	ProfilingSample* sample;
	Thread* thread;

	switch((BTrace::TProfiling)aTrace.iSubCategory)
		{

	case BTrace::ECpuFullSample:
		{
		CHECK_TRACE_DATA_WORDS(2);

		TUint32 aThread = aTrace.iData[1];
		// The thread id is aTrace.iData[1], so find or create it
		// This action can modify aTrace.iData[1], that is why we took a copy above
		thread = Thread::FindOrCreate(aTrace,1);
		if( thread )
			thread->Sampled();

		TUint32 aPC = aTrace.iData[0];

		// Always create a sample identified by the running counter ProfilingSample::iSamples
		aTrace.iData[0] = ProfilingSample::iSamples;
		sample = ProfilingSample::Create(aTrace,0);
		if( sample )
			{
			sample->SetPC( aPC );
			sample->SetThread( aThread );
			sample->SetType(BTrace::ECpuFullSample);
			}

		ProfilingSample::iLastThread = aThread;
		}
		break;

	case BTrace::ECpuOptimisedSample:
		{
		CHECK_TRACE_DATA_WORDS(1);
		TUint32 aPC = aTrace.iData[0];

		aTrace.iData[0] = ProfilingSample::iSamples;
		sample = ProfilingSample::Create(aTrace,0);
		if( sample )
			{
			sample->SetPC( aPC );
			sample->SetType( BTrace::ECpuOptimisedSample );
			sample->SetThread(ProfilingSample::iLastThread);
			}

		if( 0 != ProfilingSample::iLastThread )
			{
			thread = Thread::Find(ProfilingSample::iLastThread);
			if( thread )
				{
				thread->Sampled();
				}
			}

		}
		break;

	case BTrace::ECpuIdfcSample:
		{
		CHECK_TRACE_DATA_WORDS(1);
		TUint32 aPC = aTrace.iData[0];

		aTrace.iData[0] = ProfilingSample::iSamples;
		sample = ProfilingSample::Create(aTrace,0);

		sample->SetPC( aPC );
		sample->SetType(BTrace::ECpuIdfcSample);

		}
		break;

	case BTrace::ECpuNonSymbianThreadSample:
		{
		// No data
		aTrace.iData[0] = ProfilingSample::iSamples;
		sample = ProfilingSample::Create(aTrace,0);
		sample->SetType(BTrace::ECpuNonSymbianThreadSample);

		}
		break;

	default:
		ProfilingSampleErrors++;
		ErrorOnThisTrace = true;
		}

	ProfilingSample::iSamples++;

	}


void ReportSampleProfiling()
	{
	printf("\nREPORT: Profiling\n\n");

	TUint numSamples = ProfilingSample::iContainer.Count();
	if(!numSamples)
		{
		printf("\n        No Samples\n\n");
		return;
		}

	WarnIfError(0);


	// Print thread samples
	TUint numThreads = Thread::iContainer.Count();
	if(numThreads)
		{
		printf(" Samples by Thread\n\n");
		printf("%-11s %-8s %-8s\t%-12s\t%s\n\n", "", "TraceId", "Samples", "%", "Name");
		TUint i;
		TReal threadPercentage;
		for(i=0; i<numThreads; ++i)
			{
			Thread* thread = (Thread*)Thread::iContainer[i];

			if( thread && thread->iSamples )
				{
				Object::FullNameBuf fullName;
				thread->FullName(fullName);
				Object::TraceNameBuf name;
				thread->TraceName(name);

				threadPercentage = thread->iSamples*100.0/numSamples;

				printf("%-10s %08x %8d\t%02.2f\t'%s'\n",
							name,
							(unsigned int)thread->iTraceId,
							(unsigned int)(thread->iSamples),
							threadPercentage,
							fullName );

				}//if samples
			}//for numThreads
		}//if threads


	if(ReportLevel>0)
		{

		printf("\nAll samples\n\n%-21s %-8s %-8s\n\n", "Type", "ThreadId", "PC");

		TUint i;
		TUint fullSamples = 0;
		TUint optSamples = 0;
		TUint dfcSamples = 0;
		TUint nonSymbSamples = 0;

		for(i=0; i<numSamples; ++i)
			{
			ProfilingSample* sample = (ProfilingSample*)ProfilingSample::iContainer[i];
			switch((BTrace::TProfiling)sample->iType)
				{
				case BTrace::ECpuFullSample:
					{
					if( ReportLevel>1)
						printf("ECpuFull              %08x %08x\n",
							 (unsigned int)(sample->iThread), (unsigned int)(sample->iPC) );

					fullSamples++;
					}
					break;
				case BTrace::ECpuOptimisedSample:
					{
					if( ReportLevel>1)
						printf("ECpuOptimised         %08x %08x\n",
							 (unsigned int)(sample->iThread), (unsigned int)(sample->iPC) );

					optSamples++;
					}
					break;
				case BTrace::ECpuIdfcSample:
					{
					if( ReportLevel>1)
						printf("ECpuIdfc              %08x\n", (unsigned int)(sample->iPC) );

					dfcSamples++;
					}
					break;
				case BTrace::ECpuNonSymbianThreadSample:
					{
					if( ReportLevel>1)
						printf("ECpuNonSymbianThread\n");

					nonSymbSamples++;
					}
					break;
				}//switch
			}//for


		TReal typePercentage;

		printf("\nSamples by type\n");

		typePercentage = fullSamples * 100.0 / numSamples;
		printf(" Samples of type ECpuFullSample :\t\t%-10d\t%02.2f %%\n", fullSamples, typePercentage  );

		typePercentage = optSamples * 100.0 / numSamples;
		printf(" Samples of type ECpuOptimisedSample :\t\t%-10d\t%02.2f %%\n", optSamples, typePercentage  );

		typePercentage = dfcSamples * 100.0 / numSamples;
		printf(" Samples of type ECpuIdfcSample :\t\t%-10d\t%02.2f %%\n", dfcSamples, typePercentage  );

		typePercentage = nonSymbSamples * 100.0 / numSamples;
		printf(" Samples of type ECpuNonSymbianThreadSample :\t%-10d\t%02.2f %%\n", nonSymbSamples, typePercentage  );

		printf(" Total Samples : \t\t\t\t%d\n", numSamples );

		}//report level

	printf("\n");
	}


void PreProcessHSched(TraceRecord& aTrace)
	{
	switch((BTrace::THSched)aTrace.iSubCategory)
		{
		case BTrace::ELbDone:
			{
			CHECK_TRACE_DATA_WORDS(2);
			Thread::Find(aTrace, 0);
			break;
			}
		}
	}



//
// Trace processing
//

TraceRecord** TraceIndex = 0;
TUint TraceIndexSize = 0;
TUint32 NextTraceId = 0;
TraceRecord* LastTrace = 0;
TBool Timestamp2Present = 0;
TBool TraceDumpStarted = false;


void StartTrace()
	{
	TraceDumpStarted = false;
	TraceFormatErrors = 0;
	TraceBufferFilled = false;
	Timestamp2Present = false;
	}


TUint32 ReadTraceWord(const TUint8*& header)
	{
	TUint32 word;
	memcpy(&word, header, sizeof(TUint32));
	header += sizeof(TUint32);
	return word;
	}


TBool PreProcessTrace(TraceRecord& aTrace, const TUint8* aData)
	{
	ErrorOnThisTrace = false;
	aTrace.iError = 0;

	aTrace.iDataSize = 0; // initialise to safe value

	// process aTrace header...
	TUint traceSize = aData[BTrace::ESizeIndex];
	if(traceSize<4u || traceSize>(TUint)KMaxBTraceRecordSize)
		{
		aTrace.iError = 1;
		return false; // bad size
		}
	aTrace.iCpuNum = 0;

	TUint8 flags = aData[BTrace::EFlagsIndex];
	if(!TraceRecordId)						// first trace record...?
		flags &= ~BTrace::EMissingRecord;	// ignore missing traces before log start
	aTrace.iFlags = flags;

	TUint8 category = aData[BTrace::ECategoryIndex];
	aTrace.iCategory = category;

	TUint8 subCategory = aData[BTrace::ESubCategoryIndex];
	aTrace.iSubCategory = subCategory;

	const TUint8* header = aData+4;

	TUint32 header2 = 0;
	if(flags&BTrace::EHeader2Present)
		{
		header2 = ReadTraceWord(header);
		aTrace.iCpuNum = (TUint8)(header2>>20);
		}
	aTrace.iHeader2 = header2;
	aTrace.iCpu = TheCpus + aTrace.iCpuNum;

	// process timestamp and timestamp2...
	TUint32 ts1 = 0;
	TUint32 ts2 = 0;
	TUint64 timestamp = 0;
	if(flags&BTrace::ETimestampPresent)
		ts1 = ReadTraceWord(header);
	if(flags&BTrace::ETimestamp2Present)
		{
		Timestamp2Present = true;
		ts2 = ReadTraceWord(header);
		}
	aTrace.iTimestamp2 = ts2;
	if(flags&BTrace::ETimestampPresent)
		{
		if (Timestamp64Bit)
			{
			timestamp = ts2;
			timestamp <<= 32;
			timestamp |= ts1;
			Timestamp = timestamp;
			}
		else
			{
			timestamp = ts1;
			if(timestamp<(Timestamp&0xffffffffu))
				Timestamp += TUint64(1)<<32;
			Timestamp &= TUint64(0xffffffff)<<32;
			Timestamp |= timestamp; 
			timestamp = Timestamp;
			}
		if(!TraceRecordId)
			TimestampBase = timestamp; // record timestamp of first trace
		}
	aTrace.iTimestamp = timestamp;

	// process context...
	// coverity[assign_zero]
	aTrace.iContextID = 0;
	if(flags&BTrace::EContextIdPresent)
		{
		TUint32 contextId = ReadTraceWord(header);
		Thread* thread = Thread::Find(contextId);
		if(!thread)
			thread = new Thread(contextId);
		aTrace.iContextID = thread;
		}

	// process pc...
	TUint32 pc = 0;
	if(flags&BTrace::EPcPresent)
		pc = ReadTraceWord(header);
	aTrace.iPC = pc;

	// process extra...
	TUint32 extra = 0;
	if(flags&BTrace::EExtraPresent)
		extra = ReadTraceWord(header);
	aTrace.iExtra = extra;

	// process payload data...
	TUint headerSize = header-aData;
	aData = (TUint8*)header;
	if(headerSize>traceSize)
		{
		aTrace.iError = 1;
		return false; // bad trace record
		}
	TUint dataSize = traceSize-headerSize;
	if(dataSize>sizeof(aTrace.iData))
		{
		aTrace.iError = 1;
		return false; // bad trace record
		}
	aTrace.iDataSize = dataSize;
	memcpy(&aTrace.iData,aData,dataSize);

	// clear pre-processor specific data...
	aTrace.iDataTypes[0] = 0;
	aTrace.iDataTypes[1] = 0;
	aTrace.iDataTypes[2] = 0;
	aTrace.iDataTypes[3] = 0;
	aTrace.iCalculatedData[0] = 0;
	aTrace.iCalculatedData[1] = 0;

	// check for missing.
	if(flags & BTrace::EMissingRecord)
		{// Some trace was missing as the btrace buffer must have been filled.
		TraceBufferFilled = true;
		aTrace.iError = 1;
		return false;
		}

	// category specific processing...
	switch(aTrace.iCategory)
		{
	case BTrace::ERDebugPrintf:
	case BTrace::EKernPrintf:
	case BTrace::EPlatsecPrintf:
		if((flags&BTrace::EHeader2Present) && (header2&BTrace::EMultipartFlagMask))
			aTrace.iDataTypes[2] = EDataTypeText;
		else
			aTrace.iDataTypes[1] = EDataTypeText;
		break;
	case BTrace::EThreadIdentification:
		PreProcessThreadIdentification(aTrace); break;
	case BTrace::ECpuUsage:
		PreProcessCpuUsage(aTrace); break;
	case BTrace::EChunks:
		PreProcessChunks(aTrace); break;
	case BTrace::ECodeSegs:
		PreProcessCodeSegs(aTrace); break;
	case BTrace::EKernelMemory:
		PreProcessKernelMemory(aTrace); break;
	case BTrace::EMetaTrace:
		PreProcessMetaTrace(aTrace); break;
	case BTrace::EFastMutex:
		PreProcessFastMutex(aTrace); break;
	case BTrace::EProfiling:
		PreProcessProfiling(aTrace); break;
	case BTrace::ESymbianKernelSync:
		PreProcessSymbianKernelSync(aTrace); break;
	case BTrace::EHSched:
		PreProcessHSched(aTrace); break;
	default:
		break;
		}

	// update trace ID...
	++TraceRecordId;
	if (ErrorOnThisTrace)
		aTrace.iError = 1;
	return true;
	}


void DumpTrace(TraceRecord& aTrace)
	{
	if(!TraceDumpStarted)
		{
		// print heading...
		if(SMP)
			printf("C ");
		if(Timestamp2Present)
			printf("%10s ","TimeStamp2");
		printf("%10s ","Time");
		printf("%-8s ","PC");
		if(ReportLevel>2)
			{
			printf("%-60s ","Context");
			printf("%18s ","Category");
			printf("%24s ","SubCategory");
			}
		else
			{
			printf("%-10s ","Context");
			printf("%24s ","SubCategory");
			}
		printf("Data...\n");
		TraceDumpStarted = true;
		}

	if(aTrace.iFlags&BTrace::EMissingRecord)
		printf("MISSING TRACE RECORD(S)\n");

	// print CPU number
	if (SMP)
		{
		printf("%1d ", aTrace.iCpuNum);
		}

	// print timestamp...
	if(Timestamp2Present)
		{
		if(aTrace.iFlags&BTrace::ETimestamp2Present)
			printf("%10u ",(unsigned int)aTrace.iTimestamp2);
		else
			printf("           ");
		}

	if(aTrace.iFlags&BTrace::ETimestampPresent)
		printf("%10u ",(unsigned int)Time(aTrace.iTimestamp-TimestampBase));
	else
		printf("           ");

	// print PC...
	if(aTrace.iFlags&BTrace::EPcPresent)
		printf("%08x ",(unsigned int)aTrace.iPC);
	else
		printf("         ");

	// print context...
	if(ReportLevel>2)
		{
		Object::FullTraceNameBuf fullName;
		fullName[0] = 0;
		if(aTrace.iFlags&BTrace::EContextIdPresent)
			aTrace.iContextID->FullTraceName(fullName);
		printf("%-60s ",fullName);
		}
	else
		{
		Object::TraceNameBuf traceName;
		traceName[0] = 0;
		if(aTrace.iFlags&BTrace::EContextIdPresent)
			aTrace.iContextID->TraceName(traceName);
		printf("%-10s ",traceName);
		}
	// print trace categories...
	const char* catName = CategoryName(aTrace.iCategory);
	const char* subCatName = SubCategoryName(aTrace.iCategory,aTrace.iSubCategory);
	if(ReportLevel>2)
		printf("%18s %-24s ",catName,subCatName);
	else
		{
		if(subCatName[0])
			printf("%24s ",subCatName);
		else
			printf("%24s ",catName);
		};

	// print trace data contents...
	TUint i;
	for(i=0; i<aTrace.iDataSize; i+=4)
		{
		TUint32 data = aTrace.iData[i/sizeof(TUint32)];
		if(i<16)
			{
			// first 4 words of data may have 'type' info set during pre-processing...
			switch(aTrace.iDataTypes[i/sizeof(TUint32)])
				{
			case EDataTypeObject:
				{
				// data is an object, print "full name"[traceID]...
				Object* object = (Object*)data;
				if(ReportLevel>2)
					{
					Object::FullTraceNameBuf name;
					object->FullTraceName(name);
					printf("%s ",name);
					}
				else
					{
					Object::TraceNameBuf name;
					object->TraceName(name);
					printf("%s ",name);
					}
				}
				continue;

			case EDataTypeText:
				{
				// rest of trace is text...
				TUint8* text = (TUint8*)aTrace.iData+i;
				TUint8* textEnd = text+(aTrace.iDataSize-i);
				TUint8 buffer[256];
				TUint x=0;
				while(text<textEnd && x<sizeof(buffer)-2)
					{
					TUint8 c = *text++;
					TUint8 escape = 0;
					switch(c)
						{
					case 9: escape = 't'; break;
					case 10: escape = 'n'; break;
					case 13: escape = 'r'; break;
					default:
						if(c<' ') c = '?';
						break;
						}
					if(!escape)
						buffer[x++] = c;
					else
						{
						buffer[x++] = '\\';
						buffer[x++] = escape;
						}
					}
				buffer[x] = 0;
				printf("\"%s\" ",buffer);
				i = aTrace.iDataSize; // skip to end of data
				}
				continue;

			default:
				break;
				}
			}
		// default to print data as hex value...
		printf("%08x ",(unsigned int)data);
		}

	// print any extra data added by pre-processing...
	for(i=0; i<2; ++i)
		{
		if(aTrace.iCalculatedData[i])
			printf("{%u} ",(unsigned int)aTrace.iCalculatedData[i]);
		}

	if (aTrace.iError)
		printf(" ***ERROR***");

	// end-of-line finally!
	printf("\n");
	}


void DumpAllTraces()
	{
	printf("\nREPORT: Trace Dump\n\n");
	for(TUint i=0; i<NextTraceId; i++)
		DumpTrace(*TraceIndex[i]);
	printf("\n");
	}


void ReportErrors()
	{
	TBool errors = 	TraceFormatErrors || InterruptNestErrors || TraceFormatErrors 
		|| ChunkErrors || CodeSegErrors || FastMutexNestErrors || KernelMemoryErrors
		|| ProfilingSampleErrors;

	if(!errors)
		return;
		
	printf("\nREPORT: Trace Analysis Errors\n\n");
	if(TraceFormatErrors)
		printf("\tTrace Format Errors = %d\n",TraceFormatErrors);
	if(InterruptNestErrors)
		printf("\tInterrupt Nest Errors = %d\n",InterruptNestErrors);
	if(FastMutexNestErrors)
		printf("\tFast Mutex Nest Errors = %d\n",FastMutexNestErrors);
	if(KernelMemoryErrors)
		printf("\tKernel Memory Errors = %d\n",KernelMemoryErrors);
	if(ChunkErrors)
		printf("\tChunk Errors = %d\n",ChunkErrors);
	if(CodeSegErrors)
		printf("\tCodeSeg Errors = %d\n",CodeSegErrors);
	if(ProfilingSampleErrors)
		printf("\tProfiling Errors = %d\n",ProfilingSampleErrors);
	printf("\n");
	}


/**
The last trace record created has been preporcessed.
*/
void DoneTrace()
	{
	if(LastTrace)
		TraceIndex[NextTraceId++] = (TraceRecord*)realloc(LastTrace,sizeof(TraceHeader)+LastTrace->iDataSize);
	LastTrace = 0;
	}


/**
Create a new trace record.
*/
TraceRecord* NewTrace()
	{
	if(NextTraceId>=TraceIndexSize)
		{
		TraceIndexSize += 1024;
		TraceIndex = (TraceRecord**)realloc(TraceIndex,TraceIndexSize*sizeof(TraceIndex[0]));
		ASSERT(TraceIndex);
		}
	DoneTrace();
	LastTrace = (TraceRecord*)malloc(sizeof(TraceRecord));
	return LastTrace;
	}


/**
Delete all processed traces records.
*/
void ResetTrace()
	{
	DoneTrace();
	TUint i;
	for(i=0; i<NextTraceId; ++i)
		free(TraceIndex[i]);
	free(TraceIndex);
	TraceIndex = 0;
	TraceIndexSize = 0;
	LastTrace = 0;
	NextTraceId = 0;
	TraceRecordId = 0;
	}


void EndTrace()
	{
	DoneTrace();
	}


/**
Process an entire BTrace log capture.

@param aInput		Pointer to function which will supply raw trace data.
					Function should place up to aMaxSize bytes of data at aBuffer and return
					the number of bytes stored. Return zero to indicate end of data.
@param aReportLevel	Level of detail required of trace alaysis.
					0 = brief summary, 1 = full summary, 2 = condensed trace dump, 3 = full trace dump.
*/
void ProcessAllTrace(TUint (*aInput)(TAny* aBuffer, TUint aMaxSize),TInt aReportLevel)
	{
	ReportLevel = aReportLevel;
//	__UHEAP_MARK;
	printf("Btrace Analysis:\n");
	printf("\nTHIS TOOL IS UNOFFICIAL, UNSUPPORTED AND SUBJECT TO CHANGE WITHOUT NOTICE!\n");

	StartTrace();
	StartCpuUsage();
	StartChunks();
	StartCodeSegs();
	StartFastMutex();
	StartKernelMemory();
	StartMetaTrace();
	StartProfilingSample();

	for(; !TraceBufferFilled ;)
		{
		// read more data...
		TUint size = (*aInput)(TraceBuffer+TraceBufferSize, sizeof(TraceBuffer)-TraceBufferSize);
		if(!size)
			break;
		TraceBufferSize += size;

		// process all the complete traces in buffer...
		const TUint8* data = TraceBuffer;
		TUint sizeRemaining = TraceBufferSize;
		while(sizeRemaining>BTrace::ESizeIndex)
			{
			TUint traceSize = (data[BTrace::ESizeIndex]+3)&~3;
			if(traceSize>sizeRemaining)
				break;

			TraceRecord* trace = NewTrace();
			ASSERT(trace);
			if(!PreProcessTrace(*trace,data))
				{
				if (!TraceBufferFilled)
					{
					// bad trace, create dummy 1 byte trace record...
					memset(trace,0,sizeof(*trace));
					trace->iCategory = BTrace::EMetaTrace;
					trace->iSubCategory = KJunkTraceSubcategory;
					trace->iDataSize = 4;
					trace->iData[0] = *data;
					++TraceFormatErrors;
					ErrorOnThisTrace = true;
					traceSize = 1;
					}
				else // The buffer was filled so ignore the rest of the data
					break;
				}

			data += traceSize;
			sizeRemaining -= traceSize;
			}
		
		if (!TraceBufferFilled)
			{
			memcpy(TraceBuffer,data,sizeRemaining);
			TraceBufferSize = sizeRemaining;
			}
		else
			{
			// The trace buffer was filled so ignore the rest of the data
			// and just read whatever is left to flush it from the btrace buffer.
			while ((*aInput)(TraceBuffer, sizeof(TraceBuffer))){};
			TraceBufferSize = 0;	// reset here so a format error isn't reported
			}

		if(aReportLevel<2)
			ResetTrace(); // free up memory as we go along
		}
	EndTrace();
	EndCpuUsage();

	if(TraceBufferSize)
		{
		++TraceFormatErrors;
		ErrorOnThisTrace = true;
		}

	ReportTimeUnits();
	ReportErrors();
	if(aReportLevel>=2)
		DumpAllTraces();
	if(ReportLevel>=1 || CpuUsagePresent)
		{
		ReportProcesses();
		ReportThreads();
		}
	ReportChunks();
	ReportKernelMemory();
	ReportCodeSegs();
	ReportFastMutex();
	ReportSampleProfiling();

	ResetTrace();
	ObjectContainer::Reset();
//	__UHEAP_MARKEND;
	}


