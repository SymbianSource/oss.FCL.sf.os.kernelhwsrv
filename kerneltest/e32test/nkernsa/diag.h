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
// e32test\nkernsa\diag.h
// 
//

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*TAvailFn)(void);
typedef int (*TPollFn)(void);
typedef void (*TOutFn)(char);

typedef struct
	{
	TAvailFn	iAvail;
	TPollFn		iPoll;
	TOutFn		iOut;
	} DiagIO;

extern const DiagIO* TheIoFunctions;

extern int InputAvailable(void);
extern char GetC(void);
extern void PutC(char);

extern void PutS(const char*);
extern void PrtHex2(unsigned long);
extern void PrtHex4(unsigned long);
extern void PrtHex8(unsigned long);
extern void NewLine(void);
extern void PutSpc(void);
extern int GetAndEchoLine(char*, int, const char*);

extern int SkipSpc(const char*);
extern int CharToHex(char);
extern int ReadHex(const char*, unsigned long*);
extern int ReadRangeHex(const char*, unsigned long*, unsigned long*);

extern void DumpMemory1(const void*, const void*);
extern void DumpMemoryRange1(const void*, unsigned long, const void*);
extern void DumpStruct(const char*, const void*);

extern void RunCrashDebugger();

#ifdef __cplusplus
}
#endif

