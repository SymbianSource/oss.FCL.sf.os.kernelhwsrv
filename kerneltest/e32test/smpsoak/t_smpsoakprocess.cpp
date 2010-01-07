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
//e32test\smpsoak\t_smpsoakprocess.cpp

//  User Includes
#include "t_smpsoak.h"

#define PRINT(args)\
    if (!TestSilent)\
        test.Printf args

void ParseCmdLine();

//class for soak process and same executable(t_smpsoakprocess.exe) will be lauched with different process operation
//Example: IPC Read, IPC Write, File Process, Timer Process
class CSMPSoakProcess
    {
public:
	CSMPSoakProcess();
	~CSMPSoakProcess();
	void CreateThread(TPtrC aThreadType);
private:
    //Thread Functions
 	static TInt FileThread(TAny*);
	static TInt TimerThread(TAny*);
	static TInt MemoryThread(TAny*);
private:
   // Thread member functions
    TInt DoFileThread();
    TInt DoTimerThread();
    TInt DoMemoryThread();
    void DoCreateThread(TAny*);
    void ResumeThread();
    //IPC's
    void WriteProcess();
    void ReadProcess();
    //Thread Priority
    void SetThreadPriority();
    //Utils for soak process
	void SetSoakProcessPriority();
	void CommitChunk(RChunk& aChunk, TInt aSize);
	void ReadChunk(RChunk& aChunk, TInt aSize);
	void WriteToChunk(RChunk& aChunk, TInt aSize);
	void DeleteChunk(RChunk& aChunk);
private:
    //Thread tables
    static TThread KOOMemoryTable[];
    static TThread KFileTable[];
    static TThread KTimerTable[];
private:
    TThreadData iThreadData;
    RThread     iThread;
    TInt        iPriority;
    };
 
//Memory thread data
TThread CSMPSoakProcess::KOOMemoryTable[] =
    {   
         { _L("SMPOOMemoryThread1"), CSMPSoakProcess::MemoryThread, {{EPriorityAbsoluteLowNormal, EPriorityAbsoluteVeryLow,   EPriorityNormal, 0}, EPriorityList, KCpuAffinityAny, 0, 4, NULL, NULL,NULL}},
         { _L("SMPOOMemoryThread2"), CSMPSoakProcess::MemoryThread, {{EPriorityAbsoluteLow, EPriorityAbsoluteVeryLow,   EPriorityNormal, 0}, EPriorityList, KCpuAffinityAny, 0, 4, NULL, NULL,NULL}},
         { _L("SMPOOMemoryThread3"), CSMPSoakProcess::MemoryThread, {{EPriorityMore, EPriorityAbsoluteVeryLow, EPriorityNormal, 0}, EPriorityList, KCpuAffinityAny, 0, 4, NULL, NULL,NULL}},
         { _L("SMPOOMemoryThread4"), CSMPSoakProcess::MemoryThread, {{EPriorityAbsoluteLow, EPriorityAbsoluteVeryLow, EPriorityNormal, 0}, EPriorityList, KCpuAffinityAny, 0, 4, NULL, NULL,NULL}},
    };

//File thread data
TThread CSMPSoakProcess::KFileTable[] =
    {   
        { _L("SMPFileThread1"), CSMPSoakProcess::FileThread, {{EPriorityAbsoluteLow, EPriorityAbsoluteVeryLow,   EPriorityNormal, 0}, EPriorityList, KCpuAffinityAny, 0, 4, NULL, 11, 5}},
        { _L("SMPFileThread2"), CSMPSoakProcess::FileThread, {{EPriorityNormal, EPriorityAbsoluteVeryLow,   EPriorityNormal, 0}, EPriorityList, KCpuAffinityAny, 0, 4, NULL, 22, 10}},
        { _L("SMPFileThread3"), CSMPSoakProcess::FileThread, {{EPriorityMore, EPriorityAbsoluteVeryLow,   EPriorityNormal, 0}, EPriorityList, KCpuAffinityAny, 0, 4, NULL, 33, 15}},
        { _L("SMPFileThread4"), CSMPSoakProcess::FileThread, {{EPriorityAbsoluteVeryLow, EPriorityMore,   EPriorityNormal, 0}, EPriorityList, KCpuAffinityAny, 0, 4, NULL, 44, 20}},
    };

//Timer thread data
TThread CSMPSoakProcess::KTimerTable[] =
    {   
        { _L("SMPTimerThread1"), CSMPSoakProcess::TimerThread, {{EPriorityAbsoluteLowNormal, EPriorityAbsoluteVeryLow,   EPriorityNormal, 0}, EPriorityList, KCpuAffinityAny, 1000, 2, NULL, NULL,NULL}},
        { _L("SMPTimerThread2"), CSMPSoakProcess::TimerThread, {{EPriorityAbsoluteLow, EPriorityAbsoluteVeryLow,   EPriorityNormal, 0}, EPriorityList, KCpuAffinityAny, 1500, 2, NULL, NULL,NULL}},
    };
//Constructor
CSMPSoakProcess::CSMPSoakProcess()
    { 
    }
//Destructor
CSMPSoakProcess::~CSMPSoakProcess()
    {    
    }
//Set the process priority each time for each process
void CSMPSoakProcess::SetSoakProcessPriority()
	{
	RProcess proc;
	TInt priority;
	static TInt priorityindex = 0;
	static const TProcessPriority priorityTable[]=
		{
		EPriorityLow,
		EPriorityBackground,
		EPriorityForeground,
		EPriorityHigh
		};
	if(++priorityindex >= 4)
		priorityindex=0;
	priority = priorityTable[priorityindex];
	proc.SetPriority((TProcessPriority)priority);
	PRINT((_L("Process Priority:%d \n"),proc.Priority()));
	}
//Changes the thread priority each time time, for each thread by Random, Increment, from List, Fixed.
//pick up the priority option from thread table
void CSMPSoakProcess::SetThreadPriority()
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
         //   PRINT(_L("SetPriority List CPU %d index %d priority %d\n"),gSMPStressDrv.GetThreadCPU(&iThread),iPriority, iThreadData.threadPriorities[iPriority]);
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
          //  PRINT(_L("SetPriority Increment CPU %d priority %d\n"),gSMPStressDrv.GetThreadCPU(&iThread), iPriority);
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
           // PRINT(_L("SetPriority Random CPU %d iPriority %d\n"),gSMPStressDrv.GetThreadCPU(&iThread), iPriority);
            iThread.SetPriority((TThreadPriority)iPriority);
            break;
        }
    }
//Resume each thread
void CSMPSoakProcess::ResumeThread()
    {
    iThread.Resume();
    }
// CSMPSoakProcess Thread Creation.
// @param aThread thread table data          
void CSMPSoakProcess::DoCreateThread(TAny* aThread)
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
//Commit the chunk with aSize
void CSMPSoakProcess::CommitChunk(RChunk& aChunk, TInt aSize)
	{
	//PRINT ((_L("Commit Chunk \n")));
	test_KErrNone(aChunk.Adjust(aSize));
	}
//Write some data into the chunk 
void CSMPSoakProcess::WriteToChunk(RChunk& aChunk, TInt aSize)
	{
	TUint8 *writeaddr = aChunk.Base();
	TPtr8 write(writeaddr,aSize);
	write.Fill('S',aSize);
	write.Copy(memData);
	}
//Read the data from chunk and verify
void CSMPSoakProcess::ReadChunk(RChunk& aChunk, TInt aSize)
	{
	TUint8 *readaddr = aChunk.Base();
	TPtr8 read(readaddr,aSize);
	test_KErrNone(read.Compare(memData));
	}
//Cleaunup chunk
void CSMPSoakProcess::DeleteChunk(RChunk& aChunk)
	{
	test_KErrNone(aChunk.Adjust(0));
	}
//IPC Read operation
void CSMPSoakProcess::ReadProcess()
    {
	RTest test(_L("SMPSoakReadProcess"));
	FOREVER
		{
		// SetSoakProcessPriority();
		 gWriteSem.Wait(); //Wait for write completion
		 PRINT((_L("Read Chunk\n")));
		 ReadChunk( gChunk,KChunkSize);
		 PRINT((_L("Delete Chunk\n")));
		 DeleteChunk(gChunk);
		 gReadSem.Signal(); //Read completion
		}
    }
//IPC Write operation
void CSMPSoakProcess::WriteProcess()
	{
	RTest test(_L("SMPSoakWriteProcess"));
	FOREVER
		{
		// SetSoakProcessPriority();
		 CommitChunk( gChunk, KChunkSize);
		 PRINT((_L("Write To Chunk\n")));
		 WriteToChunk( gChunk,KChunkSize);
		 gWriteSem.Signal(); //Write completion
		 gReadSem.Wait(); //Wait for read completion
		}
	}
//File Thread - creates Dir's, Files, Fileread, Filewrite and verify
//param aSoakThread - CSMPSoakUtil pointer
TInt CSMPSoakProcess::FileThread(TAny* aSoakThread)
     {
    CSMPSoakProcess* self = (CSMPSoakProcess*)aSoakThread;
    __ASSERT_ALWAYS(self !=NULL, User::Panic(_L("CSMPSoakProcess::TimerThread Panic"),0));
    return self->DoFileThread();
     }
//Member Filethread
 TInt CSMPSoakProcess::DoFileThread()
	 {
 	 RTest test(_L("SMPFileThread"));
 	 TInt r = KErrNone;
 
 	 TFileName sessionPath;
 	 TBuf8<KFileNameLength> fileData;
 	 fileData.Copy(KFileData);
 	 RFs fs;
 	 RFile file;

 	 TBuf<KFileNameLength> filename;
 	 TBuf<KFileNameLength> directory;
 	 TBuf<KFileNameLength> tempdir;
 	 
 	//Setup Dir structure
 	 tempdir.Format(KDir,iThreadData.dirID);
  	 test_KErrNone(fs.Connect());
  	 sessionPath=KSessionPath;
 	 TChar driveLetter;
 	 
 	 //Setup Drive and Session
 	 test_KErrNone(fs.DriveToChar(EDriveD,driveLetter));
 	 sessionPath[0]=(TText)driveLetter;
 	 test_KErrNone(fs.SetSessionPath(sessionPath));
 	 test.Printf(_L("SessionPath=%S\n"),&sessionPath);
 	 directory=sessionPath;
 	 directory.Append(tempdir);
 	PRINT((_L("Dir Level =%S Creation\n"),&directory));
 	 
 	 FOREVER
 			{
 			r= fs.MkDirAll(directory);
 			test(r == KErrNone || r == KErrAlreadyExists);
 			
                //Create Number of files then write data into it.
                for (TInt i = 0; i < iThreadData.numFile; i++)
                    {	
                    filename.Format(KFile,iThreadData.dirID,i);
                    PRINT((_L("File = %S Write\n"),&filename));
                    test_KErrNone(file.Create(fs,filename,EFileWrite));
                    test_KErrNone(file.Write(fileData));
                    file.Close();
                    }
                
                //Read those files and verify it
                for (TInt i = 0; i < iThreadData.numFile; i++)
                    {	
                    TBuf8<KFileNameLength> readData;
                    filename.Format(KFile,iThreadData.dirID,i);
                    PRINT((_L("File = %S Read/Verify\n"),&filename));
                    test_KErrNone(file.Open(fs,filename,EFileRead));
                    test_KErrNone(file.Read(readData));
                    test_KErrNone(readData.Compare(fileData));
                    file.Close();
                    }
                
                //Delete files
                for (TInt i = 0; i < iThreadData.numFile; i++)
                    {	
                    filename.Format(KFile,iThreadData.dirID,i);
                    PRINT((_L("File = %S Delete\n"),&filename));
                    test_KErrNone(fs.Delete(filename));
                    }
                
                //Remove Dir's
                PRINT((_L("Dir Level =%S Removed\n"),&directory));
 				test_KErrNone(fs.RmDir(directory));
 				SetThreadPriority();
 				if (gAbort)
 				    break;
 				User::After(gPeriod);
 			}
 	 fs.Close();
 	 return 0x00;
 	 }
//Timer Thread - produces DFC's in the kernel side
//param aSoakThread - CSMPSoakUtil pointer
 TInt CSMPSoakProcess::TimerThread(TAny* aSoakThread)
     {
     CSMPSoakProcess* self = (CSMPSoakProcess*)aSoakThread;
     __ASSERT_ALWAYS(self !=NULL, User::Panic(_L("CSMPSoakProcess::TimerThread Panic"),0));
     return self->DoTimerThread();
     }
//Member TimerThread
TInt CSMPSoakProcess::DoTimerThread()
	 {
 	 RTest test(_L("SMPSoakTimerThread"));
 	 
 	 RTimer timer;
 	 test_KErrNone(timer.CreateLocal());
 	 TRequestStatus status;
 	 
 	 FOREVER
 		{
 		timer.After(status, iThreadData.delayTime*1000);
 		User::WaitForRequest(status);
 		test(status == KErrNone);
 		PRINT((_L("$")));
 		SetThreadPriority();
 		if (gAbort)
 		    break;
 		User::After(gPeriod);
 		}
 	 
 	 timer.Close();
 	 return 0x00;
	 }
 
 //OOM Thread - produces out of memory condition on SMP threads run on different cpu cores
 //param aSoakThread - this pointer
 TInt CSMPSoakProcess::MemoryThread(TAny* aSoakThread)
     {
     CSMPSoakProcess* self = (CSMPSoakProcess*)aSoakThread;
     __ASSERT_ALWAYS(self !=NULL, User::Panic(_L("CSMPSoakProcess::MemoryThread Panic"),0));
     return self->DoMemoryThread();
     }
//Memory thread member
 TInt CSMPSoakProcess::DoMemoryThread()
     {
     RTest test(_L("SMPOOMemoryThread"));
     
     static TInt memOKCount =0;
     TAny* oomheap = NULL;
     TAny* prev = NULL;
     
     //Reserve the memory in heap
     RHeap* heap;
     heap = UserHeap::ChunkHeap(NULL, KHeapMinSize, KHeapMaxiSize);
     
     //Keep produce OOM condition and inform to other threads (run on different CPU cores)
     FOREVER
         {
          TInt allocsize = KHeapMaxiSize - KHeapReserveSize;
          
          if(memOKCount == iThreadData.numThreads-1)
              allocsize = KHeapMaxiSize;
     
          prev = oomheap;
          oomheap = heap->Alloc(allocsize);
          if(oomheap == NULL)
              {
              PRINT(_L("Out Of Memory\n"));
              heap->Free(prev);
              PRINT(_L("Recover Back Memory\n")); 
              memOKCount = 0;
              ooMemSem.Signal(iThreadData.numThreads - 1);
              }
          else
             {
             ++memOKCount;
             PRINT((_L("%d:Here MemOK\n"),memOKCount));
             ooMemSem.Wait();
             }
          //Change Thread Priority
          SetThreadPriority();
          if (gAbort)
             break;
          User::After(gPeriod);
         }
     if(heap != NULL)
     heap->Close();
     return 0x00;
     }
//Create thread
 void CSMPSoakProcess::CreateThread(TPtrC aThreadType)
     {
     if (aThreadType == _L("-W"))
         {
         CSMPSoakProcess smpipcwrite;
         smpipcwrite.WriteProcess();
         }
     else if (aThreadType == _L("-R"))
         {
         CSMPSoakProcess smpipcread;
         smpipcread.ReadProcess();
         }
     else if (aThreadType == _L("-F"))
         {
         CSMPSoakProcess smpfilethread[KNumFileThreads];
         for (TInt i = 0; i < KNumFileThreads; i++)
             smpfilethread[i].DoCreateThread(&KFileTable[i]);
         for (TInt i = 0; i < KNumFileThreads; i++)
             smpfilethread[i].ResumeThread();
         }
     else if (aThreadType == _L("-T"))
         {
         CSMPSoakProcess smptimerthread[KNumTimerThreads];
         for (TInt i = 0; i < KNumTimerThreads; i++)
             smptimerthread[i].DoCreateThread(&KTimerTable[i]);
         for (TInt i = 0; i < KNumTimerThreads; i++)
             smptimerthread[i].ResumeThread();
         }
     else if (aThreadType == _L("-O"))
         {
         CSMPSoakProcess smpoomthread[KNumOOMThreads];
         for (TInt i = 0; i < KNumOOMThreads; i++)
             smpoomthread[i].DoCreateThread(&KOOMemoryTable[i]);
         for (TInt i = 0; i < KNumOOMThreads; i++)
             smpoomthread[i].ResumeThread();
         }               
     /* else
          {
          test.Printf(_L("Invalid Argument for Soak Process \n"));
          test(EFalse);
          }*/
     }
//Command line arg to launch operation specific process
void ParseCmdLine()
	{
 	TBuf<256> cmd;
 	User::CommandLine(cmd);
 	TLex	lex(cmd);
 	PRINT ((_L("Command for Process = %s\n"), cmd.PtrZ()));
 	CSMPSoakProcess smpp;
 	FOREVER
 		{
 		TPtrC  token=lex.NextToken();
 		if(token.Length()!=0)
 			{   
               if (token.Length()==0)
 		            break;  // ignore trailing whitespace
 			   else if (token.Mid(0) == _L("-b"))
 			                {
 			                test.Printf(_L("SMPSOAKPROCESS: Silent Mode\n")); 
 			                TestSilent = ETrue;
 			                lex.SkipSpaceAndMark();
 			                token.Set(lex.NextToken());
 			                test.Printf(_L("-b Thread Type = %s\n"), token.Ptr());
 			                smpp.CreateThread(token);
 			                break;
 			                }
                else if (token.Left(2) == _L("-p"))
                            {
                            test.Printf(_L("SMPSOAKPROCESS: period\n"));
                            lex.SkipSpaceAndMark();
                            token.Set(lex.NextToken());
                            TLex lexNum(token);
                            lexNum.Val(gPeriod,EDecimal);    
                            test.Printf(_L("SMPSOAKPROCESS:period in mSeconds=%d \n"),gPeriod);  
                            token.Set(lex.NextToken());
                            test.Printf(_L("-p Thread Type = %s\n"), token.Ptr());
                            smpp.CreateThread(token);
                            break;
                            }
                else
                            {
                            test.Printf(_L("-d Thread Type = %s\n"), token.Ptr());
                            smpp.CreateThread(token);
                            break;
                            }
            }
 		 break;
 		}
	}
// Child process called by (T_SMPSOAK) Main Process
TInt E32Main() 
	{
    test.Title();
    __UHEAP_MARK;
    test.Start(_L("t_SMPSoakProcess.exe"));
    test.Next(_L("Load device driver"));
    TInt r = User::LoadLogicalDevice(_L("d_smpsoak.ldd"));
    if (r == KErrNotFound)
  		{
  		PRINT (_L("Test not supported on this platform because the D_SMPSOAK.LDD Driver is Not Present\n"));
   		test(EFalse);
   		}
    
    PRINT (_L("Calling SMPStressDrv.Open\n"));
  	r = gSMPStressDrv.Open();
  	test_KErrNone(r);
  	
  	PRINT (_L("Create/Open Global Write Semaphores\n"));
    r = gWriteSem.CreateGlobal(KGlobalWriteSem,0);
   	if (r==KErrAlreadyExists)
   		{
   		r = gWriteSem.OpenGlobal(KGlobalWriteSem);
   		}
   	if (r!=KErrNone)
   		{
   		PRINT ((_L("Error- OpenGlobal Write Semaphore:%d\n"),r));
   		test(EFalse);
   		}
  
   PRINT (_L("Create/Open Global Read Semaphores\n"));
   r = gReadSem.CreateGlobal(KGlobalReadSem,0);
   if (r==KErrAlreadyExists)
	   {
   	   r = gReadSem.OpenGlobal(KGlobalReadSem);
	   }
   if (r!=KErrNone)
	   {
	   PRINT( (_L("Error- OpenGlobal Read Semaphore:%d\n"),r));
	   test(EFalse);
	   }
   
   PRINT (_L("Creating Global Chunk\n"));
   r = gChunk.CreateGlobal(KGlobalWRChunk,KChunkSize,KChunkMaxSize);
   if(r==KErrAlreadyExists)
       {
       test_KErrNone( gChunk.OpenGlobal(KGlobalWRChunk,EFalse));
       }
  
   PRINT (_L("Creating local OOM Memory semaphore\n"));
   r=ooMemSem.CreateLocal(0);
   if (r!=KErrNone)
       {
       PRINT ((_L("Error- Creating local OOM Memory semaphore:%d\n"),r));
       test(EFalse);
       }
   
   ParseCmdLine();
   
   CActiveScheduler* myScheduler = new (ELeave) CActiveScheduler();
   test(myScheduler != NULL);
   CActiveScheduler::Install(myScheduler);
   CActiveScheduler::Start();
 
   ooMemSem.Close();
   gWriteSem.Close();
   gReadSem.Close();
   gChunk.Close();
   gSMPStressDrv.Close();
   CActiveScheduler::Stop();
   __UHEAP_MARKEND;
   test.End();
   return 0x00; 
	}







