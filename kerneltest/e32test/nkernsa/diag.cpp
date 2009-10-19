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
// e32test\nkernsa\diag.cpp
// 
//

#include <nktest/diag.h>

#ifdef __cplusplus
extern "C" {
#endif

const DiagIO* TheIoFunctions;

int InputAvailable(void)
	{
	return (*TheIoFunctions->iAvail)();
	}

char GetC(void)
	{
	int c;
	do	{
		c = (*TheIoFunctions->iPoll)();
		} while (c<0);
	return (char)c;
	}

void PutC(char c)
	{
	(*TheIoFunctions->iOut)(c);
	}

void PutS(const char* s)
	{
	while (*s)
		PutC(*s++);
	}

void PrtHex(unsigned long x, int n)
	{
	if (n>8)
		n=8;
	if (n<1)
		n=1;
	x <<= (32-4*n);
	while (n--)
		{
		char c = (char)(x>>28);
		x<<=4;
		c += '0';
		if (c > '9')
			c += ('a'-'9'-1);
		PutC(c);
		}
	}

void PrtHex2(unsigned long x)
	{
	PrtHex(x, 2);
	}

void PrtHex4(unsigned long x)
	{
	PrtHex(x, 4);
	}

void PrtHex8(unsigned long x)
	{
	PrtHex(x, 8);
	}

void NewLine(void)
	{
	PutC('\r');
	PutC('\n');
	}

void PutSpc(void)
	{
	PutC(' ');
	}

int GetAndEchoLine(char* buf, int max, const char* prompt)
	{
	int n = 0;
	PutS(prompt);
	for (;;)
		{
		int c;
		do	{
			c = GetC();
			} while (c==0x0a);	// ignore LF
		if (c==0x0d || c==0x18 || c==0x03)
			{
			if (c!=0x0d)
				n = 0;
			NewLine();
			break;
			}
		else if (c==0x08)
			{
			if (n>0)
				--n;
			}
		else if (n<max-1)
			{
			buf[n++] = (char)c;
			}
		PutC((char)c);
		}
	buf[n] = 0;
	return n;
	}

int SkipSpc(const char* s)
	{
	const char* s0 = s;
	while (*s==32 || *s==9)
		++s;
	return s - s0;
	}

int CharToHex(char c)
	{
	if (c>='0' && c<='9')
		return c - '0';
	else if (c>='A' && c<='F')
		return c - 'A' + 10;
	else if (c>='a' && c<='f')
		return c - 'a' + 10;
	return -1;
	}

int ReadHex(const char* s, unsigned long* d)
	{
	const char* s0 = s;
	unsigned long x = 0;
	for (;;)
		{
		int digit = CharToHex(*s);
		if (digit < 0)
			break;
		++s;
		x = (x<<4) | ((unsigned long)digit);
		}
	*d = x;
	return s - s0;
	}

int ReadRangeHex(const char* s, unsigned long* base, unsigned long* length)
	{
	unsigned long b = 0;
	unsigned long l = 0;
	const char* s0 = s;
	char c;
	int i;
	s += SkipSpc(s);
	i = ReadHex(s, &b);
	if (i == 0)
		return 0;
	s += i;
	s += SkipSpc(s);
	c = *s;
	if (c!='+' && CharToHex(c)<0)
		return 0;
	if (c=='+')
		{
		++s;
		s += SkipSpc(s);
		}
	i = ReadHex(s, &l);
	if (i == 0)
		return 0;
	s += i;
	if (c!='+')
		{
		if (l >= b)		// inclusive end address given
			l -= (b-1);	// calculate length
		else
			l = 1;		// end < base so assume length of 1
		}
	*base = b;
	*length = l;
	return s - s0;
	}

void DumpMemory1(const void* mem, const void* disp)
	{
	int i;
	const unsigned char* s = (const unsigned char*)mem;
	PrtHex8((unsigned long)disp);
	PutC(':');
	for (i=0; i<16; ++i)
		{
		PutSpc();
		PrtHex2(s[i]);
		}
	PutSpc();
	for (i=0; i<16; ++i)
		{
		char c = s[i];
		if (c<32 || c>=127)
			c = '.';
		PutC(c);
		}
	NewLine();
	}

void DumpMemoryRange1(const void* base, unsigned long length, const void* disp)
	{
	const char* s = (const char*)base;
	const char* sd = (const char*)disp;
	do	{
		DumpMemory1(s, sd);
		s += 16;
		sd += 16;
		if (length < 16)
			length = 0;
		else
			length -= 16;
		if ((*TheIoFunctions->iPoll)() == 3)
			break;
		} while (length > 0);
	}

unsigned long _readdata(const void** pp, unsigned long bytes, int align)
	{
	unsigned long x = 0;
	unsigned long addr = (unsigned long)(*pp);
	const unsigned char* s;
	if (align)
		{
		addr += (bytes - 1);
		addr &= ~(bytes - 1);
		}
	s = (const unsigned char*)(addr + bytes);
	*pp = s;
	while (bytes--)
		x = (x<<8) | (*--s);
	return x;
	}

void DumpStruct(const char* s, const void* p)
	{
	for (;;)
		{
		char c = *s++;
		if (c == 0)
			break;
		if (c == '\n')
			PutC('\r');
		if (c != '%')
			{
			PutC(c);
			continue;
			}
		c = *s++;
		switch (c)
			{
			case '%':	PutC(c); break;
			case 'b':	PrtHex2(_readdata(&p, 1, 1)); break;
			case 'h':	PrtHex4(_readdata(&p, 2, 1)); break;
			case 'w':	PrtHex8(_readdata(&p, 4, 1)); break;
			}
		}
	}

void RunCrashDebugger()
	{
	char buf[128];
	for (;;)
		{
		int len = GetAndEchoLine(buf, 128, ".");
		(void)len;
		int i = 0;
		int err = 0;
		const char* s = buf;
		char cmd = *s++;
		switch (cmd)
			{
			case 'x':
			case 'X':
				return;
			case '\0':
				break;
			case 'm':
				{
				unsigned long mbase = 0;
				unsigned long mlen = 0;
				i = ReadRangeHex(s, &mbase, &mlen);
				if (i)
					DumpMemoryRange1((void*)mbase, mlen, (void*)mbase);
				else
					err = 2;
				break;
				}
			default:
				err = 1;
				break;
			}
		switch (err)
			{
			case 1: PutS("\r\nUnknown command\r\n"); break;
			case 2: PutS("\r\nSyntax error\r\n"); break;
			default: break;
			}
		}
	}



#ifdef __cplusplus
}
#endif

