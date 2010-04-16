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
// e32test\nkernsa\kprintf.cpp
// 
//

#define __E32CMN_H__
#include <nktest/utils.h>
#include "nk_priv.h"

#undef EXPORT_C
#define EXPORT_C /* */

class TVersion
	{
public:
	TInt8 iMajor;
	TInt8 iMinor;
	TInt16 iBuild;
	};

class TDesC;

#include <e32rom.h>

extern void DebugPrint(const char*, int);

/**
Returns the active debug mask obtained by logically ANDing the global debug mask
in the super page with the per-thread debug mask in the current DThread object.

If the current thread is not a symbian OS thread the global debug mask is used.

Only supports the first 32 global debug trace bits.

@return The debug mask.
*/
extern "C" {
extern TLinAddr RomHeaderAddress;
}
EXPORT_C TInt KDebugMask()
	{
	const TRomHeader& rh = *(const TRomHeader*)RomHeaderAddress;
	return rh.iTraceMask[0];
	}



/**
Returns the state (ETrue or EFalse) of given bit in the active debug mask
which is obtained by logically ANDing the global debug mask in the super page 
with the per-thread debug mask in the current DThread object.

If the current thread is not a symbian OS thread the global debug mask is used.

@return The state of the debug mask bit number.
*/

EXPORT_C TBool KDebugNum(TInt aBitNum)
	{
#if 1
	const TRomHeader& rh = *(const TRomHeader*)RomHeaderAddress;
	TInt m = 0;

	// special case for KALWAYS
	if (aBitNum == KALWAYS)
		{
		m = rh.iTraceMask[0] ||
			rh.iTraceMask[1] ||
			rh.iTraceMask[2] ||
			rh.iTraceMask[3] ||
			rh.iTraceMask[4] ||
			rh.iTraceMask[5] ||
			rh.iTraceMask[6] ||
		    rh.iTraceMask[7];
		}
	else if  ( (aBitNum > KMAXTRACE) || (aBitNum < 0) )
		m = 0;
	else
		{
		TInt index = aBitNum >> 5;
		m = rh.iTraceMask[index];
		m &= 1 << (aBitNum & 31);
		}

	return (m != 0);
#else
	return 1;
#endif
	}

extern "C" unsigned int strlen(const char* s)
	{
	const char* s0 = s;
	while(*s++) {}
	return s - s0 - 1;
	}

int appendstr(char* out, int outlen, const char* s)
	{
	if (!s)
		s = "NULL";
	char* d = out + outlen;
	while(*s)
		*d++ = *s++;
	return d - out;
	}

int AppendNumBase10U(char* out, unsigned int val, int width, int fill)
	{
	int len = 10;
	if (val < 10)
		len = 1;
	else if (val < 100)
		len = 2;
	else if (val < 1000)
		len = 3;
	else if (val < 10000)
		len = 4;
	else if (val < 100000)
		len = 5;
	else if (val < 1000000)
		len = 6;
	else if (val < 10000000)
		len = 7;
	else if (val < 100000000)
		len = 8;
	else if (val < 1000000000)
		len = 9;
	int w = (len < width) ? width : len;
	char* d = out + w;
	do	{
		*--d = (char)(48 + val%10);
		val /= 10;
		} while(val);
	for (; d > out; *--d = (char)fill ) {}
	return w;
	}

int AppendNumBase10S(char* out, int sval, int width, int fill)
	{
	int sign = (sval<0) ? 1 : 0;
	unsigned val = sign ? unsigned(-sval) : unsigned(sval);
	int len = 10;
	if (val < 10)
		len = 1;
	else if (val < 100)
		len = 2;
	else if (val < 1000)
		len = 3;
	else if (val < 10000)
		len = 4;
	else if (val < 100000)
		len = 5;
	else if (val < 1000000)
		len = 6;
	else if (val < 10000000)
		len = 7;
	else if (val < 100000000)
		len = 8;
	else if (val < 1000000000)
		len = 9;
	if (sign) ++len;
	int w = (len < width) ? width : len;
	char* d = out + w;
	do	{
		*--d = (char)(48 + val%10);
		val /= 10;
		} while(val);
	if (sign) *--d = '-';
	for (; d > out; *--d = (char)fill ) {}
	return w;
	}

int AppendNumBase16(char* out, unsigned int val, int width, int fill)
	{
	int len = 8;
	if (val < 0x10)
		len = 1;
	else if (val < 0x100)
		len = 2;
	else if (val < 0x1000)
		len = 3;
	else if (val < 0x10000)
		len = 4;
	else if (val < 0x100000)
		len = 5;
	else if (val < 0x1000000)
		len = 6;
	else if (val < 0x10000000)
		len = 7;
	int w = (len < width) ? width : len;
	char* d = out + w;
	do	{
		char c = (char)(48 + (val&15));
		if (c>'9') c+=0x07;
		*--d = c;
		val >>= 4;
		} while(val);
	for (; d > out; *--d = (char)fill ) {}
	return w;
	}

int AppendNumBase16L(char* out, Uint64 val, int width, int fill)
	{
	TUint vl = (TUint)val;
	TUint vh = (TUint)(val>>32);
	TInt l = 0;
	if (vh)
		{
		l = AppendNumBase16(out, vh, width-8, fill);
		l += AppendNumBase16(out+l, vl, 8, fill);
		}
	else
		l = AppendNumBase16(out, vl, width, fill);
	return l;
	}



/**
Formats and appends text to the specified narrow descriptor without making any
executive calls.

The function takes a format string and a variable number of arguments. The
format specifiers in the format string are used to interpret and the arguments.

Format directives have the following syntax:
@code
<format-directive> ::= 
	"%" [<padding-character>] [<field-width>] [<long-flag>] <conversion-specifier>
@endcode

If a field width is specified and the width of the formatted field is less
than this width, then the field is padded with the padding character.
The only supported padding characters are ' ' (default) and '0'.

The long flag specifier ('l') modifies the semantic of the conversion
specifier as explained below.

The possible values for the conversion specifiers, the long flag and the way in
which arguments are interpreted, are as follows:
@code
d	Interpret the argument as a TInt decimal representation
ld	NOT SUPPORTED - use lx instead
u	Interpret the argument as a TUint decimal representation
lu	NOT SUPPORTED - use lx instead
x	Interpret the argument as a TUint hexadecimal representation
X	As above
lx	Interpret the argument as a Uint64 hexadecimal representation
lX	As above
c	Interpret the argument as a character
s	Interpret the argument as a pointer to narrow C string
ls	Interpret the argument as a pointer to narrow C string
S 	Interpret the argument as a pointer to narrow descriptor or NULL
lS	NOT SUPPORTED - use S instead
O	Interpret the argument as a pointer to DObject or NULL 
	Generates the object full name or 'NULL'
o	Interpret the argument as a pointer to DObject or NULL
	Generates the object name or 'NULL'
M	Interpret the argument as a pointer to a fast mutex or NULL
	Generates the name, if this is a well-known fast mutex, address otherwise
m	Interpret the argument as a pointer to a fast semaphore or NULL
	Generates the owning thread name, if this is a well-known fast semaphore, address otherwise
T	Interpret the argument as a pointer to a nanothread or NULL 
	Generates the full name, if this is a Symbian OS thread, address otherwise
C	Interpret the argument as a pointer to a DCodeSeg or NULL
	Generates the filename and module version number
@endcode

The function can be called from the interrupt context, but extreme caution is advised as it
may require a lot of stack space and interrupt stacks are very small.

@param aDes 	Narrow descriptor that must be big-enough to hold result
@param aFmt 	The format string
@param aList 	A variable number of arguments to be converted to text as dictated by the format string

@pre Calling thread can be either in a critical section or not.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked
@pre No fast mutex can be held.
@pre Call in any context.
@pre Suitable for use in a device driver

@panic The set of panics that can be raised when appending data to descriptors.

@see   TDes8
*/
EXPORT_C TInt AppendFormat(char* aOut, const char* aFmt, VA_LIST aList)
	{
#define NEXT_FMT(c,p) if (((c)=*(p)++)==0) return outLen;

	TInt outLen = 0;
	while(outLen>=0)
		{
		char c;
		NEXT_FMT(c,aFmt);
		if (c=='%')
			{
			char fill=' ';
			TInt width=0;
			TBool long_arg=EFalse;
			TBool ok=ETrue;
			NEXT_FMT(c,aFmt);
			if (c=='0')
				{
				fill='0';
				NEXT_FMT(c,aFmt);
				}
			while(c>='0' && c<='9')
				{
				width=width*10+c-'0';
				NEXT_FMT(c,aFmt);
				}
			if (c=='l')
				{
				long_arg=ETrue;
				NEXT_FMT(c,aFmt);
				}
			switch(c)
				{
				case 'd':
					{
					if (long_arg)
						ok=EFalse;
					else
						{
						TInt val=VA_ARG(aList,TInt);
						char* d = aOut + outLen;
						outLen += AppendNumBase10S(d, val, width, fill);
						}
					break;
					}
				case 'u':
					{
					if (long_arg)
						ok=EFalse;
					else
						{
						TUint val=VA_ARG(aList,TUint);
						char* d = aOut + outLen;
						outLen += AppendNumBase10U(d, val, width, fill);
						}
					break;
					}
				case 'x':
				case 'X':
					{
					if (long_arg)
						{
						Uint64 val=VA_ARG(aList,Uint64);
						char* d = aOut + outLen;
						outLen += AppendNumBase16L(d, val, width, fill);
						}
					else
						{
						TUint val=VA_ARG(aList,TUint);
						char* d = aOut + outLen;
						outLen += AppendNumBase16(d, val, width, fill);
						}
					break;
					}
				case 's':
					{
					const char* s = VA_ARG(aList,const char*);
					outLen = appendstr(aOut, outLen, s);
					break;
					}
				case 'M':		// fast mutex
					{
					NFastMutex* pM=VA_ARG(aList,NFastMutex*);
					outLen = appendstr(aOut, outLen, "M");
					if (!pM)
						outLen = appendstr(aOut, outLen, 0);
					else if (pM==&TheScheduler.iLock)
						outLen = appendstr(aOut, outLen, "SYSLOCK");
					else
						outLen += AppendNumBase16(aOut+outLen, (TUint)pM, 8, '0');
					break;
					}
				case 'm':		// fast semaphore
					{
					NFastSemaphore* pS=VA_ARG(aList,NFastSemaphore*);
					outLen = appendstr(aOut, outLen, "S");
					if (!pS)
						outLen = appendstr(aOut, outLen, 0);
					else
						outLen += AppendNumBase16(aOut+outLen, (TUint)pS, 8, '0');
					break;
					}
				case 'T':		// NKERN thread
					{
					NThread* pN=VA_ARG(aList,NThread*);
#ifdef __SMP__
					if (pN && pN->iNThreadBaseSpare8)
						outLen = appendstr(aOut, outLen, (const char*)pN->iNThreadBaseSpare8);
#else
					if (pN && pN->iSpare8)
						outLen = appendstr(aOut, outLen, (const char*)pN->iSpare8);
#endif
					else
						{
						outLen = appendstr(aOut, outLen, "T");
						if (!pN)
							outLen = appendstr(aOut, outLen, 0);
						else
							outLen += AppendNumBase16(aOut+outLen, (TUint)pN, 8, '0');
						}
					break;
					}
#ifdef __SMP__
				case 'G':		// NKERN thread group
					{
					NThreadGroup* pG=VA_ARG(aList,NThreadGroup*);
//					if (pG && pG->iNThreadBaseSpare8)
//						outLen = appendstr(aOut, outLen, (const char*)pG->iNThreadBaseSpare8);
//					else
						{
						outLen = appendstr(aOut, outLen, "G");
						if (!pG)
							outLen = appendstr(aOut, outLen, 0);
						else
							outLen += AppendNumBase16(aOut+outLen, (TUint)pG, 8, '0');
						}
					break;
					}
#endif
				case 'c':
					c=(char)VA_ARG(aList,TUint);
					// fall through
				default:
					ok=EFalse;
					break;
				}
				if (ok)
					continue;
			}
		aOut[outLen++]=c;
		}
	return outLen;
	}


/**
Prints a formatted string on the debug port.

The function uses Kern::AppendFormat() to do the formatting.

Although it is safe to call this function from an ISR, it polls the output
serial port and may take a long time to complete, invalidating any
real-time guarantee.

If called from an ISR, it is possible for output text to be intermingled
with other output text if one set of output interrupts or preempts another.

Some of the formatting options may not work inside an ISR.

Be careful not to use a string that is too long to fit onto the stack.

@param aFmt The format string. This must not be longer than 256 characters.
@param ...	A variable number of arguments to be converted to text as dictated
            by the format string.

@pre Calling thread can either be in a critical section or not.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked
@pre No fast mutex can be held.
@pre Call in any context.
@pre Suitable for use in a device driver

@see Kern::AppendFormat()
*/
extern "C" void puts(const char* s);
extern "C" void prthex8(TUint);
EXPORT_C void KPrintf(const char* aFmt, ...)
	{
	extern TUint32 __tr();

	char printBuf[256];
	VA_LIST list;
	VA_START(list,aFmt);
	int c = AppendFormat(printBuf+2,aFmt,list) + 2;
	printBuf[c++] = 13;
	printBuf[c++] = 10;
	printBuf[0] = __trace_cpu_num()+48;
	printBuf[1] = 58;

	if (NKern::Crashed())
		{
		DebugPrint(printBuf,c);
		return;
		}

	// Handle BTrace first...
	TUint category = BTrace::EKernPrintf;
	TInt result = BTraceContextBig(category,0,0,printBuf,c);

	NThread* csThread = 0;
	NThread* curr = NKern::CurrentThread();
	if (curr && NKern::CurrentContext() == NKern::EThread && !NKern::KernelLocked())
		{
		csThread = curr;
		NKern::_ThreadEnterCS();
		}
	if (!result)
		{
		DebugPrint(printBuf,c);
		}
	if (csThread)
		{
		NKern::_ThreadLeaveCS();
		}
	}



/******************************************************************************
 * BTRACE SUPPORT
 ******************************************************************************/

#define BTRACE_INCLUDE_TIMESTAMPS

TAny* BTraceBufferBase[KMaxCpus];
TAny* BTraceBufferEnd[KMaxCpus];
TAny* BTraceBufferPtr[KMaxCpus];	// next free position
TBool BTraceBufferWrap[KMaxCpus];
TBool BTraceActive;

//const TUint KBTraceBufferSize = 16 * 1024 * 1024;
const TUint KBTraceBufferSize = 1 * 1024 * 1024;
const TUint KBTraceSlotSize = 128;

__ASSERT_COMPILE(KBTraceSlotSize >= (TUint)KMaxBTraceRecordSize);

TBool HandleBTrace(TUint32 aHeader,TUint32 aHeader2,const TUint32 aContext,const TUint32 a1,const TUint32 a2,const TUint32 a3,const TUint32 aExtra,const TUint32 aPc)
	{
#ifdef __SMP__
	// Header 2 always present and contains CPU number
	// If Header2 not originally there, add 4 to size
	TInt cpu = NKern::CurrentCpu();
	if (!(aHeader&(BTrace::EHeader2Present<<BTrace::EFlagsIndex*8)))
		aHeader += (4<<BTrace::ESizeIndex*8) + (BTrace::EHeader2Present<<BTrace::EFlagsIndex*8), aHeader2=0;
	aHeader2 = (aHeader2 &~ BTrace::ECpuIdMask) | (cpu<<20);
#else
	TInt cpu = 0;
#endif
#ifdef BTRACE_INCLUDE_TIMESTAMPS
	// Add timestamp to trace...
#if defined(__EPOC32__) && defined(__CPU_X86)
	aHeader += 8<<BTrace::ESizeIndex*8;
	aHeader |= BTrace::ETimestampPresent<<BTrace::EFlagsIndex*8 | BTrace::ETimestamp2Present<<BTrace::EFlagsIndex*8;
	TUint64 timeStamp = fast_counter();
#else
	aHeader += 4<<BTrace::ESizeIndex*8;
	aHeader |= BTrace::ETimestampPresent<<BTrace::EFlagsIndex*8;
	TUint32 timeStamp = NKern::FastCounter();
#endif
#endif
	TUint size = (aHeader+3)&0xfc;

#if !defined(__SMP__) || !defined(__USE_BTRACE_LOCK__)
	TInt irq = NKern::DisableAllInterrupts();
#endif

	TUint32* src;
	TUint32* dst = (TUint32*)BTraceBufferPtr[cpu];

	if (!BTraceActive)
		goto trace_off;

	BTraceBufferPtr[cpu] = ((TUint8*)BTraceBufferPtr[cpu]) + KBTraceSlotSize;
	if (BTraceBufferPtr[cpu] >= BTraceBufferEnd[cpu])
		{
		BTraceBufferPtr[cpu] = BTraceBufferBase[cpu];
		BTraceBufferWrap[cpu] = TRUE;
		}

	size >>= 2; // we are now counting words, not bytes

	if (dst+size > (TUint32*)BTraceBufferEnd[cpu])
		goto trace_dropped;

	{
	// store first word of trace...
	TUint w = aHeader;
	--size;
	*dst++ = w;

#ifndef __SMP__
	if (aHeader&(BTrace::EHeader2Present<<(BTrace::EFlagsIndex*8)))
#endif
		{
		w = aHeader2;
		--size;
		*dst++ = w;
		}

#ifdef BTRACE_INCLUDE_TIMESTAMPS
	// store timestamp...
#if defined(__EPOC32__) && defined(__CPU_X86)
	--size;
	*dst++ = TUint32(timeStamp);
	--size;
	*dst++ = TUint32(timeStamp>>32);
#else
	--size;
	*dst++ = timeStamp;
#endif
#endif

	if(aHeader&(BTrace::EContextIdPresent<<(BTrace::EFlagsIndex*8)))
		{
		w = aContext;
		--size;
		*dst++ = w;
		}

	if(aHeader&(BTrace::EPcPresent<<(BTrace::EFlagsIndex*8)))
		{
		w = aPc;
		--size;
		*dst++ = w;
		}

	if(aHeader&(BTrace::EExtraPresent<<(BTrace::EFlagsIndex*8)))
		{
		w = aExtra;
		--size;
		*dst++ = w;
		}

	// store remaining words of trace...
	if(size)
		{
		w = a1;
		--size;
		*dst++ = w;
		if(size)
			{
			w = a2;
			--size;
			*dst++ = w;
			if(size)
				{
				if(size==1)
					{
					w = a3;
					*dst++ = w;
					}
				else
					{
					src = (TUint32*)a3;
					do
						{
						w = *src++;
						--size;
						*dst++ = w;
						}
					while(size);
					}
				}
			}
		}
	}

#if !defined(__SMP__) || !defined(__USE_BTRACE_LOCK__)
	NKern::RestoreInterrupts(irq);
#endif
	return ETrue;


trace_dropped:
#if !defined(__SMP__) || !defined(__USE_BTRACE_LOCK__)
	NKern::RestoreInterrupts(irq);
#endif
	return ETrue;

trace_off:
#if !defined(__SMP__) || !defined(__USE_BTRACE_LOCK__)
	NKern::RestoreInterrupts(irq);
#endif
	return ETrue;
//	return EFalse;
	}


TBool SBTraceData::CheckFilter2(TUint32)
	{
	return TRUE;
	}

void InitBTraceHandler()
	{
	TInt cpu;
#ifdef __SMP__
	TInt ncpus = NKern::NumberOfCpus();
#else
	TInt ncpus = 1;
#endif
	for (cpu=0; cpu<ncpus; ++cpu)
		{
		BTraceBufferBase[cpu] = malloc(KBTraceBufferSize);
		TEST_OOM(BTraceBufferBase[cpu]);
		BTraceBufferEnd[cpu] = ((TUint8*)BTraceBufferBase[cpu]) + KBTraceBufferSize;

		TUint8* p = (TUint8*)BTraceBufferBase[cpu];

		BTraceBufferPtr[cpu] = p;
		BTraceBufferWrap[cpu] = FALSE;

		TEST_PRINT2("BTraceBufferBase[%d] = %08x", cpu, BTraceBufferBase[cpu]);
		TEST_PRINT2("BTraceBufferEnd[%d]  = %08x", cpu, BTraceBufferEnd[cpu]);
		TEST_PRINT2("BTraceBufferPtr[%d]  = %08x", cpu, BTraceBufferPtr[cpu]);
		}

	SBTraceData& traceData = BTraceData;
	traceData.iHandler = &HandleBTrace;
//	traceData.iFilter[BTrace::EKernPrintf] = 1;
	traceData.iFilter[BTrace::EThreadIdentification] = 1;
	traceData.iFilter[BTrace::ECpuUsage] = 1;
	traceData.iFilter[0xdd] = 1;
//	memset(traceData.iFilter, 1, sizeof(traceData.iFilter));
	}

void DumpBTraceBuffer()
	{
	BTraceActive = FALSE;

	TInt cpu;
#ifdef __SMP__
	TInt ncpus = NKern::NumberOfCpus();
#else
	TInt ncpus = 1;
#endif
	char x[64];
	for (cpu=0; cpu<=ncpus; ++cpu)
		{
		memset(x, cpu+0xc0, sizeof(x));
		DebugPrint(x, sizeof(x));
		if (cpu == ncpus)
			break;
		const char* b = (const char*)BTraceBufferBase[cpu];
		const char* e = (const char*)BTraceBufferEnd[cpu];
		const char* f = (const char*)BTraceBufferPtr[cpu];
		const char* p = BTraceBufferWrap[cpu] ? f : b;
		if (!p || (!BTraceBufferWrap[cpu] && p==f))
			continue;
		do	{
			TInt size = *(const TUint8*)p;
			size = (size + 3) &~ 3;
			DebugPrint(p, size);
			p+=KBTraceSlotSize;
			if (p==e)
				p = b;
			} while (p!=f);
		}

	SBTraceData& traceData = BTraceData;
	memset(traceData.iFilter, 0, sizeof(traceData.iFilter));
	traceData.iHandler = 0;
	TEST_PRINT("\r\n\nBTrace Dump Complete");
	}

void StartBTrace()
	{
	BTraceActive = TRUE;
	}

void StopBTrace()
	{
	BTraceActive = FALSE;
	}

TInt KCrazySchedulerEnabled()
	{
	return 0;
	}
