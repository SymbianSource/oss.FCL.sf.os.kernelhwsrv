// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\smpsoak\t_smpsoak.h

#if (!defined __T_SMPSOAK_H__)
#define __T_SMPSOAK_H__

#define __E32TEST_EXTENSION__
//  EPOC includes
#include <e32test.h>
#include <u32hal.h>
#include <f32file.h>
#include <e32math.h>

//  User Includes
#include "d_smpsoak.h"

TBool TestSilent = EFalse;
TBool Period = EFalse;
TBool timeout = EFalse;
TBool ThreadPriorityLow = EFalse;
TBuf<25> gCmdLine;
_LIT(KCmdLineProcess,            "%s");
_LIT(KCmdLineBackground,         "-b %s");
_LIT(KCmdLinePeriod,             "-p %d %s");

TUint gPeriod = 10000;

#define PRINT(args)\
    if (!TestSilent)\
        test.Printf args

//Global Literals
_LIT(KGlobalWriteSem, 	"GlobalWriteSem");
_LIT(KGlobalReadSem, 	"GlobalReadSem");
_LIT(KGlobalWRChunk, 	"GlobalWRChunk");
_LIT(KSessionPath, 		"?:\\SMPSOAK-TST\\");
_LIT(KDir, 				"Dir%d\\");
_LIT(KFile, 			"\\SMPSOAK-TST\\Dir%d\\File%d.txt");
_LIT(KFileData, 		"A$$$BCDEFGHIJKLMNOPQRSTUVWXY$$$Z");
const TUint8* pattern = (TText8*)("A11$$222BCDEUVWXY££££$$$Z");


//Global's used between the process
const TUint32 KCpuAffinityAny=0xffffffffu;

static RSemaphore gWriteSem;
static RSemaphore gReadSem;
static RChunk   gChunk;
static volatile TBool gAbort = EFalse; // Set true when escape key pressed
static RSMPSoak gSMPStressDrv;
static RSemaphore gSwitchSem;

//Chunk Allocation IPC Read/Write operations
static const TInt KChunkMaxSize      = 0x01000000; //16 MB
static const TInt KChunkSize         = 0x400000;  //4MB
TBuf8<KChunkSize> memData;

//Heap Allocations for OOM Threads
const TInt KHeapMaxiSize      = 0x200000; //2MB
const TInt KHeapReserveSize   = 0x100000; //1MB
RSemaphore ooMemSem;

//For Thread Creation
const TInt KHeapMinSize= 0x1000;
const TInt KHeapMaxSize= 0x1000;
const TInt KTimerPeriod = 10000;
const TInt KRandSeed= 1234;
//Used by File thread's
const TInt KFileNameLength = 100;
const TInt KPriorityOrder = 4;
static RTest test(_L("T_SMPSoak"));
//Enum's for Memory Thread Operations
enum
	{
	EChunkNone,
	EChunkNormalThread,
	EChunkDisconnectedThread,
	EChunkDoubleEndedThread,
	EChunkNormalProcess,
	EChunkDisconnectedProcess,
	EChunkDoubleEndedProcess,
	};
//Process Priority
enum
	{
	EpriorityFixed,
	EPriorityList,
	EPriorityIncrement,
	EPriorityRandom,
	};
//Memory table structure for Memory thread
struct TMemory
	{
	TPtrC globalChunkName;
	TInt chunkType;
	TInt initialBottom;
	TInt initialTop;
	TInt maxSize;
	};

struct TChunkInfo
	{
	RChunk chunk;
	TInt lastBottom;
	TInt lastTop;
	};
enum
    {   
    KNumThreads      = 7,
    KNumProcess      = 4,
    KNumFileThreads  = 4,     
    KNumTimerThreads = 2,    
    KNumOOMThreads   = 4,
    KNumChunks = 13
    };
TInt NumProcess = KNumProcess;
//Device information for device thread
_LIT(KDevices,"CDZ");
_LIT(KDevLdd1,"ecomm.ldd");
_LIT(KDevLdd1Name,"comm");
_LIT(KDevLdd2,"elocd.ldd");
_LIT(KDevLdd2Name,"LocDrv");
_LIT(KDevLdd3,"enet.ldd");
_LIT(KDevLdd3Name,"Ethernet");
_LIT(KDevLdd4,"esoundsc.ldd");
_LIT(KDevLdd4Name,"SoundSc");

//Thread data for each thread
struct	TThreadData
	{
	TInt threadPriorities[4];
	TInt threadPriorityChange;
	TUint32 cpuAffinity;
	TInt delayTime;
	TInt numThreads;
	TAny *listPtr;
	TInt dirID;
	TInt numFile;
	};

struct TThread
	{
	TPtrC threadName;
	TThreadFunction threadFunction;
	TThreadData threadData;
	};

struct TProcess
	{
	TPtrC processFileName;
	TPtrC operation;
	TUint32 cpuAffinity;
	};

inline void ShowHelp()
    {
    PRINT(_L("***************************************\n"));
    PRINT(_L("The following are immediate commands\n"));
    PRINT(_L("-l        run includes Out of Memory thread tests \n"));
    PRINT(_L("-b        run in silent mode\n"));
    PRINT(_L("-t nn     test run with timeout in seconds\n"));
    PRINT(_L("-p nnnn   period for each thread to sleep in iteration\n"));
    PRINT(_L("-h        show this help\n"));
    PRINT(_L("Esc       to shutdown\n"));
    PRINT(_L("***************************************\n"));
    }
#endif /* __T_SMPSOAK_H__ */
		
