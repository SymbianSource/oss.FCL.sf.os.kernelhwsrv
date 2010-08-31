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
// e32test\smpsoak\t_smpsoak.cpp

//  User Includes
#include <e32hal.h>
#include "t_smpsoak.h"

void ParseCommandLine ();

// Global Variables
static TInt gPageSize;
//Timeout 2 Minutes
static TUint gTimeout = 120;

//class for smpsoak thread and it creates memory, device, timer and spin threads.
class CSMPSoakThread
	{
public:
	CSMPSoakThread();
	~CSMPSoakThread();
	void CreateThread();
	void ResumeThread();
	void CreateChildProcess(TInt aIndex);
	void ResumeChildProcess();
	void TerminateChildProcess();	
private:
    //Thread Functions
	static TInt SMPStressMemoryThread(TAny*);
	static TInt SMPStressDeviceThread(TAny*);
	static TInt SMPStressTimerThread(TAny*);
	static TInt SMPStressSpinThread(TAny*);
	//Thread Priority
	void DoCreateThread(TAny*);
	void SetThreadPriority();
private:
    //Utils for memory thread
	void CreateChunk(TChunkInfo * aChunkInfo, TMemory * aMemoryTablePtr);
	void CommitChunk(TChunkInfo * aChunkInfo, TMemory * aMemoryTablePtr);
	void WriteReadChunk(TChunkInfo * aChunkInfo, TMemory * aMemoryTablePtr);
private:
    //Memebers for threads 
    TInt DoSMPStressMemoryThread();
    TInt DoSMPStressDeviceThread();
    TInt DoSMPStressTimerThread();
    TInt DoSMPStressSpinThread();
private:
    TThreadData iThreadData;
    RProcess    iProcess;
    RThread     iThread;
    TInt        iPriority;
private:
// Thread Data for each thread- low priority
static TThread KThreadTableLow[];
// Thread Data for each thread- high priority
static TThread KThreadTableHigh[];
//Process Data for each process
static const TProcess KProcessTable[];
//Memory table for memory thread
static const TMemory KMemoryTable[];
//Device table for device thread
static const TDesC* KDeviceTable[];

	};
TThread CSMPSoakThread::KThreadTableLow[] =
    {
        { _L("Memory Thread"), CSMPSoakThread::SMPStressMemoryThread, {{EPriorityAbsoluteLow, EPriorityAbsoluteVeryLow,   EPriorityNormal, 0}, EPriorityList, KCpuAffinityAny, 250, 1, (TAny *)&KMemoryTable, NULL, NULL}},
		{ _L("Device Thread"), CSMPSoakThread::SMPStressDeviceThread, {{EPriorityAbsoluteLow, EPriorityAbsoluteVeryLow, EPriorityNormal, 0}, EPriorityList, KCpuAffinityAny, 300, 1, &KDeviceTable, NULL, NULL}},
		{ _L("Spin Thread 0"), CSMPSoakThread::SMPStressSpinThread, {{EPriorityAbsoluteVeryLow, EPriorityNormal,   EPriorityNormal, EPriorityNormal}, EPriorityList, KCpuAffinityAny, 200, 0, NULL, NULL, NULL}},
		{ _L("Spin Thread 1"), CSMPSoakThread::SMPStressSpinThread, {{EPriorityNormal, EPriorityAbsoluteVeryLow,   EPriorityNormal, EPriorityNormal}, EPriorityList, KCpuAffinityAny, 300, 0, NULL, NULL, NULL}},
		{ _L("Spin Thread 2"), CSMPSoakThread::SMPStressSpinThread, {{EPriorityNormal, EPriorityNormal,   EPriorityAbsoluteVeryLow, EPriorityNormal}, EPriorityList, KCpuAffinityAny, 400, 0, NULL, NULL, NULL}},
		{ _L("Spin Thread 3"), CSMPSoakThread::SMPStressSpinThread, {{EPriorityNormal, EPriorityNormal,   EPriorityAbsoluteLow, EPriorityAbsoluteVeryLow}, EPriorityList, KCpuAffinityAny, 500, 0, NULL, NULL, NULL}},
		{ _L("Timer Thread"), CSMPSoakThread::SMPStressTimerThread, {{EPriorityNormal, 0, 0, 0}, EPriorityList, KCpuAffinityAny, 1000, 4, NULL}},
    };
TThread CSMPSoakThread::KThreadTableHigh[] =
 {
        { _L("Memory Thread"), CSMPSoakThread::SMPStressMemoryThread, {{EPriorityAbsoluteLow, EPriorityAbsoluteVeryLow,   EPriorityNormal, 0}, EPriorityList, KCpuAffinityAny, 250, 1, (TAny *)&KMemoryTable, NULL, NULL}},
        { _L("Device Thread"), CSMPSoakThread::SMPStressDeviceThread, {{EPriorityAbsoluteLow, EPriorityAbsoluteVeryLow, EPriorityNormal, 0}, EPriorityList, KCpuAffinityAny, 300, 1, &KDeviceTable, NULL, NULL}},
        { _L("Spin Thread 0"), CSMPSoakThread::SMPStressSpinThread, {{EPriorityAbsoluteVeryLow, EPriorityNormal,   EPriorityNormal, EPriorityNormal}, EPriorityList, KCpuAffinityAny, 200, 0, NULL, NULL, NULL}},
        { _L("Spin Thread 1"), CSMPSoakThread::SMPStressSpinThread, {{EPriorityNormal, EPriorityAbsoluteVeryLow,   EPriorityNormal, EPriorityNormal}, EPriorityList, KCpuAffinityAny, 300, 0, NULL, NULL, NULL}},
        { _L("Spin Thread 2"), CSMPSoakThread::SMPStressSpinThread, {{EPriorityNormal, EPriorityNormal,   EPriorityAbsoluteVeryLow, EPriorityNormal}, EPriorityList, KCpuAffinityAny, 400, 0, NULL, NULL, NULL}},
        { _L("Spin Thread 3"), CSMPSoakThread::SMPStressSpinThread, {{EPriorityNormal, EPriorityNormal,   EPriorityAbsoluteLow, EPriorityAbsoluteVeryLow}, EPriorityList, KCpuAffinityAny, 500, 0, NULL, NULL, NULL}},
        { _L("Timer Thread"), CSMPSoakThread::SMPStressTimerThread, {{EPriorityNormal, 0, 0, 0}, EPriorityList, KCpuAffinityAny, 1000, 4, NULL}},
    };
const TProcess CSMPSoakThread::KProcessTable[] =
    {
        { _L("t_smpsoakprocess.exe"), _L("-W"), KCpuAffinityAny},
        { _L("t_smpsoakprocess.exe"), _L("-R"), KCpuAffinityAny},
        { _L("t_smpsoakprocess.exe"), _L("-F"), KCpuAffinityAny},
        { _L("t_smpsoakprocess.exe"), _L("-T"), KCpuAffinityAny},
        { _L("t_smpsoakprocess.exe"), _L("-O"), KCpuAffinityAny},
    };
const TMemory CSMPSoakThread::KMemoryTable[] =
    {
        {_L(""), EChunkNormalThread, 0, 10, 100 },
        {_L("Global Chunk 1"), EChunkNormalThread, 0, 20, 200 },
        {_L(""), EChunkDisconnectedThread, 3, 30, 300 },
        {_L("Global Chunk 2"), EChunkDisconnectedThread, 4, 40, 400 },
        {_L(""), EChunkDoubleEndedThread, 5, 50, 500 },
        {_L("Global Chunk 3"), EChunkDoubleEndedThread, 6, 60, 600 },
        {_L(""), EChunkNormalProcess, 0, 10, 100 },
        {_L("Global Chunk 4"), EChunkNormalProcess, 0, 20, 200 },
        {_L(""), EChunkDisconnectedProcess, 3, 30, 300 },
        {_L("Global Chunk 5"), EChunkDisconnectedProcess, 4, 40, 400 },
        {_L(""), EChunkDoubleEndedProcess, 5, 50, 500 },
        {_L("Global Chunk 6"), EChunkDoubleEndedProcess, 6, 60, 600 },
        {_L(""), EChunkNone, 0, 0, 0 },
    };
const TDesC* CSMPSoakThread::KDeviceTable[] =
    {
    &KDevices, &KDevLdd1, &KDevLdd1Name, &KDevLdd2, &KDevLdd2Name, &KDevLdd3, &KDevLdd3Name,
    &KDevLdd4, &KDevLdd4Name, NULL
    };

//Constructor
CSMPSoakThread::CSMPSoakThread()
    { 
    }
//Destructor
CSMPSoakThread::~CSMPSoakThread()
    {    
    }
//All child process creation
void CSMPSoakThread::CreateChildProcess(TInt aIndex)
    {
    if(TestSilent)  
            gCmdLine.Format(KCmdLineBackground,(KProcessTable[aIndex].operation).Ptr());
    else if (Period)
        gCmdLine.Format(KCmdLinePeriod,gPeriod,(KProcessTable[aIndex].operation).Ptr());
    else
        gCmdLine.Format(KCmdLineProcess,(KProcessTable[aIndex].operation).Ptr());
    
    TInt r = iProcess.Create(KProcessTable[aIndex].processFileName,gCmdLine);
    test_KErrNone(r);
    iProcess.SetPriority(EPriorityLow);
    gSMPStressDrv.ChangeThreadAffinity(&iThread, KProcessTable[aIndex].cpuAffinity);
    PRINT ((_L("SetProcessPriority  CPU %d Priority %d\n"),gSMPStressDrv.GetThreadCPU(&iThread), iProcess.Priority()));
    }
//Terminate process when user press "Esc key"
void CSMPSoakThread::ResumeChildProcess()
    {
    iProcess.Resume();
    }
//Terminate process when user press "Esc key"
void CSMPSoakThread::TerminateChildProcess()
    {
    iProcess.Kill(KErrNone);
    }
//Changes the thread priority each time time, for each thread by Random, Increment, from List, Fixed.
//pick up the priority option from thread table
void CSMPSoakThread::SetThreadPriority()
    {
    static TInt64 randSeed = KRandSeed;
    static const TThreadPriority priorityTable[]=
        {
        EPriorityMuchLess, EPriorityLess, EPriorityNormal, EPriorityMore, EPriorityMuchMore,
        EPriorityRealTime, EPriorityRealTime, EPriorityAbsoluteVeryLow, EPriorityAbsoluteLowNormal,
        EPriorityAbsoluteLow, EPriorityAbsoluteBackgroundNormal, EPriorityAbsoluteBackground,
        EPriorityAbsoluteForegroundNormal, EPriorityAbsoluteForeground, EPriorityAbsoluteHighNormal, EPriorityAbsoluteHigh
        };
    TInt priorityIndex = 0;
    switch (iThreadData.threadPriorityChange)
        {
        case EpriorityFixed:
            break;

        case EPriorityList:
            if (++iPriority >= KPriorityOrder)
                iPriority = 0;
            if (iThreadData.threadPriorities[iPriority] == 0)
                iPriority = 0;
          //  PRINT(_L("SetPriority List CPU %d index %d priority %d\n"),gSMPStressDrv.GetThreadCPU(&iThread),iPriority, iThreadData.threadPriorities[iPriority]);
            iThread.SetPriority((TThreadPriority)iThreadData.threadPriorities[iPriority]);
            break;

        case EPriorityIncrement:
            while (priorityTable[priorityIndex] <= iPriority)
                {
                priorityIndex++;
                }
            iPriority = priorityTable[priorityIndex];
            if (iPriority > iThreadData.threadPriorities[2])
                iPriority = iThreadData.threadPriorities[1];
          //  PRINT(_L("SetPriority Increment CPU %d iPriority %d\n"),gSMPStressDrv.GetThreadCPU(&iThread), iPriority);
            iThread.SetPriority((TThreadPriority)iPriority);
            break;

        case EPriorityRandom:
            iPriority = Math::Rand(randSeed) % (iThreadData.threadPriorities[2] - iThreadData.threadPriorities[1] + 1);
            iPriority += iThreadData.threadPriorities[1];
            while (priorityTable[priorityIndex] < iPriority)
                {
                priorityIndex++;
                }
            iPriority = priorityTable[priorityIndex];
         //   PRINT(_L("SetPriority Random CPU %d priority %d\n"),gSMPStressDrv.GetThreadCPU(&iThread), iPriority);
            iThread.SetPriority((TThreadPriority)iPriority);
            break;
        }
    }
//Resume each thread
void CSMPSoakThread::ResumeThread()
    {
    iThread.Resume();
    }
//thread creation
void CSMPSoakThread::CreateThread()
    {
    CSMPSoakThread* smpthread = new CSMPSoakThread[KNumThreads];
    for (TInt i = 0; i < KNumThreads ; i++)
        {
        if(ThreadPriorityLow)
            {
            PRINT ((_L("Thread Table - Priority Low \n")));
            smpthread[i].DoCreateThread(&KThreadTableLow[i]);
            }
        else
            {
            PRINT ((_L("Thread Table - Priority High \n")));
            smpthread[i].DoCreateThread(&KThreadTableHigh[i]);
            }
        }
    PRINT (_L("Resuming all threads\n"));
    for (TInt i = 0; i < KNumThreads; i++)
           smpthread[i].ResumeThread();
    }
/**
 * CSMPSoakThread Thread Creation.
 * @param aIndex to exercise each row(thread) in the thread table          
 *
 * @return  N/A
 *
 * @pre     Initialize thread Table values
 * @post    None
 */
void CSMPSoakThread::DoCreateThread(TAny* aThread)
    {
    //Initialize each thread data
       iThreadData = ((TThread*)aThread)->threadData;
       test.Next(_L("Create Thread"));
       PRINT ((_L("%s   CPU affinity %d  Priority %d\n"),((TThread*)aThread)->threadName.Ptr(),iThreadData.cpuAffinity,iThreadData.threadPriorities[0]));
       TInt r = iThread.Create(((TThread*)aThread)->threadName, ((TThread*)aThread)->threadFunction, KDefaultStackSize, KHeapMinSize, KHeapMaxSize,(TAny*)this);
       test_KErrNone(r);
       if (iThreadData.threadPriorityChange == EPriorityList)
           {
           iPriority = 0;
           }
       else
           {
           iPriority = iThreadData.threadPriorities[0];
           }
       iThread.SetPriority((TThreadPriority)iThreadData.threadPriorities[0]);
       //Set the thread CPU Affinity
       gSMPStressDrv.ChangeThreadAffinity(&iThread, iThreadData.cpuAffinity);
      }
//Create Chunk - different types 
void CSMPSoakThread::CreateChunk (TChunkInfo * aChunkInfo, TMemory * aMemoryTablePtr)
	{
	//RDebug::Print(_L("Creating chunk name %s type %d bottom %d top %d max %d\n"),aMemoryTablePtr->globalChunkName.Ptr(),aMemoryTablePtr->chunkType,aMemoryTablePtr->initialBottom,aMemoryTablePtr->initialTop,aMemoryTablePtr->maxSize);
	TOwnerType ownerType = EOwnerProcess;
	aChunkInfo->lastBottom = aMemoryTablePtr->initialBottom;
	aChunkInfo->lastTop = aMemoryTablePtr->initialTop;
	switch (aMemoryTablePtr->chunkType)
		{
		case EChunkNormalThread:
			ownerType = EOwnerThread;			// drop through to create chunk
		case EChunkNormalProcess:
			if (aMemoryTablePtr->globalChunkName.Length())
				{
				test_KErrNone(aChunkInfo->chunk.CreateGlobal(aMemoryTablePtr->globalChunkName,aMemoryTablePtr->initialTop*gPageSize,aMemoryTablePtr->maxSize*gPageSize,ownerType));
				}
			else
				{
				test_KErrNone(aChunkInfo->chunk.CreateLocal(aMemoryTablePtr->initialTop*gPageSize,aMemoryTablePtr->maxSize*gPageSize,ownerType));
				}
			aChunkInfo->lastBottom = 0;			// ensure that this is zero
			break;

		case EChunkDisconnectedThread:
			ownerType = EOwnerThread;			// drop through to create chunk
		case EChunkDisconnectedProcess:
			if (aMemoryTablePtr->globalChunkName.Length())
				{
				test_KErrNone(aChunkInfo->chunk.CreateDisconnectedGlobal(aMemoryTablePtr->globalChunkName,aMemoryTablePtr->initialBottom*gPageSize,aMemoryTablePtr->initialTop*gPageSize,aMemoryTablePtr->maxSize*gPageSize,ownerType));
				}
			else
				{
				test_KErrNone(aChunkInfo->chunk.CreateDisconnectedLocal(aMemoryTablePtr->initialBottom*gPageSize,aMemoryTablePtr->initialTop*gPageSize,aMemoryTablePtr->maxSize*gPageSize,ownerType));
				}
			break;

		case EChunkDoubleEndedThread:
			ownerType = EOwnerThread;			// drop through to create chunk
		case EChunkDoubleEndedProcess:
			if (aMemoryTablePtr->globalChunkName.Length())
				{
				test_KErrNone(aChunkInfo->chunk.CreateDoubleEndedGlobal(aMemoryTablePtr->globalChunkName,aMemoryTablePtr->initialBottom*gPageSize,aMemoryTablePtr->initialTop*gPageSize,aMemoryTablePtr->maxSize*gPageSize,ownerType));
				}
			else
				{
				test_KErrNone(aChunkInfo->chunk.CreateDoubleEndedLocal(aMemoryTablePtr->initialBottom*gPageSize,aMemoryTablePtr->initialTop*gPageSize,aMemoryTablePtr->maxSize*gPageSize,ownerType));
				}
			break;
		}
	}
//Commit chunk
void CSMPSoakThread::CommitChunk (TChunkInfo * aChunkInfo, TMemory * aMemoryTablePtr)
	{
	TInt commitPages;

	switch (aMemoryTablePtr->chunkType)
		{
		case EChunkNormalThread:
		case EChunkNormalProcess:
			if (aChunkInfo->lastTop < (aMemoryTablePtr->maxSize - 1))
				{
				aChunkInfo->lastTop += (aMemoryTablePtr->maxSize - 1 - aChunkInfo->lastTop) / 2 + 1;
				//PRINT(_L("Adjust chunk memory - top %d pagesize %d\n"),aChunkInfo->lastTop,gPageSize);
				test_KErrNone(aChunkInfo->chunk.Adjust(aChunkInfo->lastTop*gPageSize));
				}
			break;

		case EChunkDisconnectedThread:
		case EChunkDisconnectedProcess:
			commitPages = ((aChunkInfo->lastTop - aChunkInfo->lastBottom) / 2) + 1;
			//PRINT(_L("Decommitting from bottom %d of %d pages\n"),aChunkInfo->lastBottom,commitPages);
			test_KErrNone(aChunkInfo->chunk.Decommit(aChunkInfo->lastBottom*gPageSize,commitPages * gPageSize));
			if ((aChunkInfo->lastBottom > 0) && (aChunkInfo->lastTop <= aMemoryTablePtr->initialTop))
				{
				aChunkInfo->lastTop = aChunkInfo->lastBottom + commitPages - 1;
				aChunkInfo->lastBottom /= 2;
				commitPages = aChunkInfo->lastTop - aChunkInfo->lastBottom + 1;
				}
			else
				{
				if (aChunkInfo->lastTop < (aMemoryTablePtr->maxSize -1))
					{
					if (aChunkInfo->lastTop <= aMemoryTablePtr->initialTop)
						{
						aChunkInfo->lastBottom = aMemoryTablePtr->initialTop + 1;
						}
					else
						{
						aChunkInfo->lastBottom = aChunkInfo->lastTop + 1;
						}
					commitPages = ((aMemoryTablePtr->maxSize - aChunkInfo->lastBottom) / 2) + 1;
					aChunkInfo->lastTop = aChunkInfo->lastBottom + commitPages - 1;
					}
				else
					{
					commitPages = 0;
					}
				}
			if (commitPages)
				{
				//PRINT(_L("Commit chunk memory bottom %d size %d pages\n"),aChunkInfo->lastBottom,commitPages);
				test_KErrNone(aChunkInfo->chunk.Commit(aChunkInfo->lastBottom*gPageSize,commitPages*gPageSize));
				}
		break;

		case EChunkDoubleEndedThread:
		case EChunkDoubleEndedProcess:
			if (aChunkInfo->lastBottom > 0 || aChunkInfo->lastTop < (aMemoryTablePtr->maxSize - 1))
				{
				if (aChunkInfo->lastBottom > 0)
					{
					aChunkInfo->lastBottom--;
					}
				if (aChunkInfo->lastTop < (aMemoryTablePtr->maxSize - 1))
					{
					aChunkInfo->lastTop++;
					}
			//	PRINT(_L("Adjust Double Ended bottom %d top %d\n"),aChunkInfo->lastBottom,aChunkInfo->lastTop);
				test_KErrNone(aChunkInfo->chunk.AdjustDoubleEnded(aChunkInfo->lastBottom*gPageSize,aChunkInfo->lastTop*gPageSize));
				}
			break;
		}
	}
//Write then read chunk
void CSMPSoakThread::WriteReadChunk (TChunkInfo * aChunkInfo, TMemory * aMemoryTablePtr)
	{
	if (aChunkInfo->lastTop < (aMemoryTablePtr->maxSize - 1))
		{
		TInt chunkSize = aChunkInfo->lastTop*gPageSize - aChunkInfo->lastBottom*gPageSize;
		//RDebug::Print(_L("WriteReadChunk Last Top %d lastBottom %d\n"),aChunkInfo->lastTop,aChunkInfo->lastBottom);
		TUint8 *writeaddr = aChunkInfo->chunk.Base()+ aChunkInfo->lastBottom*gPageSize;
		TPtr8 write(writeaddr,chunkSize);
		write.Copy(pattern,sizeof(pattern));
		test_KErrNone(Mem::Compare(writeaddr,sizeof(pattern),pattern,sizeof(pattern)));
		}
	}
//Memory Thread : will do memory associated operation
//param aSmp - CSMPSoakUtil pointer
TInt CSMPSoakThread::SMPStressMemoryThread(TAny* aSmp)
    {
    CSMPSoakThread* self = (CSMPSoakThread*)aSmp;
     __ASSERT_ALWAYS(self !=NULL, User::Panic(_L("CSMPSoakThread::SMPStressMemoryThread Panic"),0));
    return self->DoSMPStressMemoryThread();
    }
// Member for thread function
TInt CSMPSoakThread::DoSMPStressMemoryThread()
	{
	RTest test(_L("SMPStressMemoryThread"));
	test.Start(_L("SMPStressMemoryThread"));
	
	TMemory *memoryTablePtr;
	TChunkInfo chunkTable[KNumChunks];
	TInt ctIndex = 0;
	test_KErrNone(UserHal::PageSizeInBytes(gPageSize));

	FOREVER
		{
		SetThreadPriority();

		if (gAbort)
			break;

		memoryTablePtr = (TMemory *) (iThreadData.listPtr);
		ctIndex = 0;
		
		//Create different type of chunks and write/read/verfiy it
		while (memoryTablePtr->chunkType != EChunkNone)
			{
			PRINT((_L("Create Chunk")));
			CreateChunk (&chunkTable[ctIndex],memoryTablePtr);

			PRINT(_L("Write and Read Chunk"));
			WriteReadChunk (&chunkTable[ctIndex],memoryTablePtr);

			ctIndex++;
			memoryTablePtr++;
			}
		
		//Commit different type of chunks
		TBool anyCommit;
		do
			{
			anyCommit = EFalse;
			memoryTablePtr = (TMemory *) (iThreadData.listPtr);
			ctIndex = 0;
			while (memoryTablePtr->chunkType != EChunkNone)
				{
				//Commit Chunks
				PRINT((_L("Commit Chunk Memory")));
				PRINT ((_L("CommitChunk %d bottom %d top %d\n"),ctIndex,memoryTablePtr->initialBottom,memoryTablePtr->initialTop));
				CommitChunk (&chunkTable[ctIndex],memoryTablePtr);
				anyCommit = ETrue;
				
				//Write into Chunks
				WriteReadChunk (&chunkTable[ctIndex],memoryTablePtr);
				PRINT((_L("Write Read Chunk Size %d\n"), (memoryTablePtr->initialTop) - (memoryTablePtr->initialBottom)));
				ctIndex++;
				memoryTablePtr++;
				}
			}
		while (anyCommit);
		
		//Close the Chunks
		memoryTablePtr = (TMemory *) (iThreadData.listPtr);
		ctIndex = 0;
		while (memoryTablePtr->chunkType != EChunkNone)
			{
			chunkTable[ctIndex].chunk.Close();

			ctIndex++;
			memoryTablePtr++;
			}
		User::After(gPeriod);
		}
	test.End();
	test.Close();
	return 0x00;
	}
//Device Thread : will do device associated operation
//param aSmp - CSMPSoakUtil pointer
TInt CSMPSoakThread::SMPStressDeviceThread(TAny* aSmp)
    {
    CSMPSoakThread* self = (CSMPSoakThread*)aSmp;
     __ASSERT_ALWAYS(self !=NULL, User::Panic(_L("CSMPSoakThread::SMPStressDeviceThread Panic"),0));
    return self->DoSMPStressDeviceThread();
    }
// Member for thread function
TInt CSMPSoakThread::DoSMPStressDeviceThread()
	{
	RTest test(_L("SMPStressDeviceThread"));
	test.Start(_L("SMPStressDeviceThread"));
	
	RTimer timer;
	RFs session;
	TFileName sessionPath;

	test_KErrNone(timer.CreateLocal());
	TRequestStatus s;

	TDesC** ptrDevices =  (TDesC**) (iThreadData.listPtr);
	PRINT ((_L("Devices  Number %d [%s]\n"), ptrDevices[0]->Length(), ptrDevices[0]->Ptr()));
	for (TInt i = 1; ptrDevices[i] ; i++)
		PRINT ((_L("LDD%d=%s "),i,ptrDevices[i]->Ptr()));
	PRINT (_L("\n"));

	FOREVER
		{
		for (TInt i = 0; i < ptrDevices[0]->Length(); i++)
			{
			TText driveLetter = (*ptrDevices[0])[i];
			PRINT ((_L("Device %c\n"),driveLetter));

			test_KErrNone(session.Connect());

			sessionPath=(_L("?:\\SESSION_TEST\\"));
			sessionPath[0]=driveLetter;
			test_KErrNone(session.SetSessionPath(sessionPath));

			TInt driveNumber;
			test_KErrNone(session.CharToDrive(driveLetter, driveNumber));

			TBuf<64> fileSystemName;
			test_KErrNone(session.FileSystemName(fileSystemName,driveNumber));

			PRINT ((_L("File System Name %s\n"),fileSystemName.PtrZ()));

			TDriveInfo driveInfo;
			test_KErrNone(session.Drive(driveInfo, driveNumber));

			TVolumeInfo volumeInfo;
			test_KErrNone(session.Volume(volumeInfo, driveNumber));

			session.Close();
			}
		for (TInt i = 1; ptrDevices[i] ; i += 2)
			{
			RDevice device;

			TInt r = User::LoadLogicalDevice(*ptrDevices[i]);
			if (r != KErrNone && r != KErrAlreadyExists)
				{
				test.Printf(_L("LDD %S not present\n"), ptrDevices[i]);
				continue;
				}

			test_KErrNone(device.Open(*ptrDevices[i+1]));

			TBuf8<64> deviceCaps;
			device.GetCaps(deviceCaps);

			TVersion deviceVersion;
			device.QueryVersionSupported(deviceVersion);

			device.Close();
			}
		SetThreadPriority();
		timer.After(s, iThreadData.delayTime*1000);
		User::WaitForRequest(s);
		test (s == KErrNone);

		if (gAbort)
			break;
		User::After(gPeriod);
		}
	timer.Close();
	PRINT((_L("SMPStressDeviceThread MyTimer.Cancel() called\n")));
	test.End();
	test.Close();
	return 0x00;
	}
//Spin Thread : will do thread sync 
//param aSmp - CSMPSoakUtil pointer
TInt CSMPSoakThread::SMPStressSpinThread(TAny* aSmp)
    {
    CSMPSoakThread* self = (CSMPSoakThread*)aSmp;
     __ASSERT_ALWAYS(self !=NULL, User::Panic(_L("CSMPSoakThread::SMPStressSpinThread Panic"),0));
    return self->DoSMPStressSpinThread();
    }
// Member for thread function
TInt CSMPSoakThread::DoSMPStressSpinThread()
	{
	RTest test(_L("SMPStressSpinThread"));
	test.Start(_L("SMPStressSpinThread"));

	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds loopTimeMicroSeconds;
	PRINT (_L("SMPStressSpinThread\n"));
	FOREVER
		{
		SetThreadPriority();
		gSwitchSem.Wait();
		startTime.UniversalTime();
		do
		{
			endTime.UniversalTime();
			loopTimeMicroSeconds = endTime.MicroSecondsFrom(startTime);
		}while (loopTimeMicroSeconds <= iThreadData.delayTime*1000);

		if (gAbort)
			break;
		User::After(gPeriod);
		}
	test.End();
	test.Close();
	return 0x00;
	}
//Timer Thread : Timer operation and  thread sync 
//param aSmp - CSMPSoakUtil pointer
TInt CSMPSoakThread::SMPStressTimerThread(TAny* aSmp)
    {
    CSMPSoakThread* self = (CSMPSoakThread*)aSmp;
     __ASSERT_ALWAYS(self !=NULL, User::Panic(_L("CSMPSoakThread::SMPStressTimerThread Panic"),0));
    return self->DoSMPStressTimerThread();
    }
// Member for thread function
TInt CSMPSoakThread::DoSMPStressTimerThread()
	{
	RTest test(_L("SMPStressTimerThread"));
	test.Start(_L("SMPStressTimerThread"));

	PRINT (_L("SMPStressTimerThread\n"));
	RTimer timer;
	test_KErrNone(timer.CreateLocal());
	TRequestStatus s;

	FOREVER
		{
		timer.After(s, iThreadData.delayTime*1000);
		User::WaitForRequest(s);
		test (s == KErrNone);
		PRINT ((_L("*")));
		gSwitchSem.Signal(iThreadData.numThreads);

		if (gAbort)
			break;
		User::After(gPeriod);
		}
	timer.Cancel();
	PRINT((_L("SMPStressTimerThread MyTimer.Cancel() called\n")));
	test.End();
	test.Close();
	return 0x00;
	}
// CActive class to monitor KeyStrokes from User
class CActiveConsole : public CActive
	{
public:
	CActiveConsole();
	~CActiveConsole();
	void GetCharacter();
	static TInt Callback(TAny* aCtrl);
	static CPeriodic* TimerL();
private:
	// Defined as pure virtual by CActive;
	// implementation provided by this class.
	virtual void DoCancel();
	// Defined as pure virtual by CActive;
	// implementation provided by this class,
	virtual void RunL();
	void ProcessKeyPressL(TChar aChar);
private:
    
	};
// Class CActiveConsole
CActiveConsole::CActiveConsole()
	: CActive(EPriorityHigh)
	{
	CActiveScheduler::Add(this);
	}

CActiveConsole::~CActiveConsole()
	{
	Cancel();
	}
CPeriodic* CActiveConsole::TimerL()
    {
    return(CPeriodic::NewL(EPriorityNormal));
    }
// Callback function for timer expiry
TInt CActiveConsole::Callback(TAny* aControl)
	{
	return KErrNone;
	}

void CActiveConsole::GetCharacter()
	{
	test.Console()->Read(iStatus);
	SetActive();
	}

void CActiveConsole::DoCancel()
	{
	PRINT(_L("CActiveConsole::DoCancel\n"));
	test.Console()->ReadCancel();
	}

void CActiveConsole::ProcessKeyPressL(TChar aChar)
	{
	if (aChar == EKeyEscape)
		{
		PRINT(_L("CActiveConsole: ESC key pressed -> stopping active scheduler...\n"));
		gAbort = ETrue;
		CActiveScheduler::Stop();
		return;
		}
	aChar.UpperCase();
	GetCharacter();
	}

void CActiveConsole::RunL()
	{
	ProcessKeyPressL(static_cast<TChar>(test.Console()->KeyCode()));
	}

// CActiveTimer class to monitor timeout expiry
class CActiveTimer : public CActive
    {
public:
    CActiveTimer();
    ~CActiveTimer();
    void Delay(TTimeIntervalMicroSeconds32 aDelay);
private:
    RTimer iTimer;
    // Defined as pure virtual by CActive;
    // implementation provided by this class.
    virtual void DoCancel();
    // Defined as pure virtual by CActive;
    // implementation provided by this class,
    virtual void RunL();
   
    };
// Class CActiveConsole
CActiveTimer::CActiveTimer()
    : CActive(EPriorityHigh)
    {
    CActiveScheduler::Add(this);
    User::LeaveIfError(iTimer.CreateLocal());
    }

CActiveTimer::~CActiveTimer()
    {
    Cancel();
    iTimer.Close();
    }


void CActiveTimer::Delay(TTimeIntervalMicroSeconds32 aDelay)
    {
    iTimer.After(iStatus, aDelay);
    SetActive();
    }

void CActiveTimer::DoCancel()
    {
    iTimer.Cancel();
    }

void CActiveTimer::RunL()
    {
    PRINT(_L("CActiveTimer: Application runtime expired..."));
    gAbort = ETrue;
    CActiveScheduler::Stop();
    return;
    }

//T_SMPSOAK Entry Point
TInt E32Main()
	{
	test.Title();
	__UHEAP_MARK;
	test.Start(_L("t_smpsoak.exe"));
	
	// When running as a stand alone test, 
	// there needs to be a timeout
	timeout = ETrue;

	ParseCommandLine();
	if (gAbort)
		return 0x00;

	PRINT (_L("Load device driver\n"));
	TInt r = User::LoadLogicalDevice(_L("d_smpsoak.ldd"));
	if (r == KErrNotFound)
		{
		PRINT (_L("Test not supported on this platform because the D_SMPSOAK.LDD Driver is Not Present\n"));
		test(EFalse);
		}
	PRINT (_L("Calling SMPStressDrv Open\n"));
	r = gSMPStressDrv.Open();
	test_KErrNone(r);

	PRINT (_L("Creating our local semaphore\n"));
	r=gSwitchSem.CreateLocal(0);
	test_KErrNone(r);

	CSMPSoakThread smpthread;
	PRINT ((_L("Creating all threads =%d\n"),KNumThreads));
	smpthread.CreateThread();
			
	CSMPSoakThread *smpprocess= new CSMPSoakThread[NumProcess];
	PRINT ((_L("Creating all process =%d\n"),NumProcess));
	for (TInt i = 0; i < NumProcess; i++)
	    smpprocess[i].CreateChildProcess(i);
	
	PRINT (_L("Resuming all process \n"));
	for (TInt i = 0; i < NumProcess; i++)
	    smpprocess[i].ResumeChildProcess();
	
	PRINT (_L("Starting ActiveScheduler\n"));
	test.Next(_L("Press ESC Key to Shutdown SMPSoak...\n"));
	CActiveScheduler* myScheduler = new (ELeave) CActiveScheduler();
	test(myScheduler != NULL);
	CActiveScheduler::Install(myScheduler);
	
	CPeriodic* theTimer=NULL;
	TRAPD(ret,theTimer=CActiveConsole::TimerL())
	test_KErrNone(ret);
	theTimer->Start(0,KTimerPeriod,TCallBack(CActiveConsole::Callback));
	if(timeout)
	    {
	    CActiveTimer* myActiveTimer = new CActiveTimer();
	    test(myActiveTimer != NULL);
	    myActiveTimer->Delay(gTimeout*1000000);
	    }
	CActiveConsole* myActiveConsole = new CActiveConsole();
	test(myActiveConsole != NULL);
	myActiveConsole->GetCharacter();
	CActiveScheduler::Start();
	if (gAbort)
			{
			PRINT (_L("gAbort TRUE \n"));
			for (TInt i = 0; i < NumProcess; i++)
			smpprocess[i].TerminateChildProcess();
			delete[] smpprocess;
			delete theTimer;
			gSMPStressDrv.Close();
			gSwitchSem.Close();
			return 0;
			}
	__UHEAP_MARKEND;
	test.End();
	return 0;
	}
void ParseCommandLine()
	{
	TBuf<256> args;
	User::CommandLine(args);
	TLex	lex(args);
	PRINT ((_L("****Command line = %s\n"), args.PtrZ()));

	FOREVER
		{
		TPtrC  token=lex.NextToken();
		if(token.Length()!=0)
			{
                if (token.Length()==0)
			        break;  // ignore trailing whitespace
                else if (token.Mid(0) == _L("-h"))
				{
                    PRINT (_L("T_SMPSOAK.EXE Usage Options:\n"));
                    PRINT (_L("Type t_smpsoak.exe -h\n"));
					ShowHelp();
					gAbort = ETrue;
					break;
				}
				else if (token.Mid(0) == _L("-l"))
				{
                    //Read OOM entry from KProcessTable and run
                    test.Printf(_L("SMPSOAK:lowmem\n"));
                    NumProcess = KNumProcess+1;
                    break;
				}
				else if (token.Mid(0) == _L("-b"))
				{
                    test.Printf(_L("SMPSOAK: Test Silent Mode\n")); 
                    ThreadPriorityLow = ETrue;
                    TestSilent = ETrue;
					// If we have tests running in the background
					// we want an endless loop
					timeout = EFalse;
                    break;
				}
				else if (token.Left(2) == _L("-t"))
				{
				    test.Printf(_L("SMPSOAK:Timeout\n"));
				    lex.SkipSpaceAndMark();
				    token.Set(lex.NextToken());
				    TLex lexNum(token);
				    lexNum.Val(gTimeout,EDecimal);   
				    test.Printf(_L("Timeout in Seconds=%u \n"),gTimeout);  
				    timeout = ETrue;
                    break;
				}
				else if (token.Left(2) == _L("-p"))
				{
                    test.Printf(_L("SMPSOAK:period\n"));
                    lex.SkipSpaceAndMark();
                    token.Set(lex.NextToken());
                    TLex lexNum(token);
                    lexNum.Val(gPeriod,EDecimal);   
				    test.Printf(_L("period in mSeconds=%d \n"),gPeriod);  
				    Period = ETrue;
				    break;
				}
				else
				{
                    test.Printf(_L("Error- Invalid SMPSOAK CMD Line Argument"));
				  	break;
				}
			}
		break;
		}
	}
