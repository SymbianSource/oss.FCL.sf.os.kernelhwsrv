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
// e32test\t_smpsoakspin.cpp
//

#define __E32TEST_EXTENSION__
#include <e32svr.h>
#include <e32test.h>
#include <u32hal.h>
#include <f32file.h>


#include "d_smpsoak.h"

#define PRINT(string) if (!gQuiet) test.Printf(string)
#define PRINT1(string,param) if (!gQuiet) test.Printf(string,param)
#define TESTNEXT(string) if (!gQuiet) test.Next(string)

#define DEBUG_PRINT(__args)		test.Printf __args

//------------globals---------------------
LOCAL_D RTest test(_L("T_SMPSOAKSPIN"));
LOCAL_D TBool gQuiet = EFalse;
LOCAL_D TBool gAbort = EFalse;

TInt		TestCpuCount = 0;

const TInt KTimerPeriod = 10000;

const TInt KHeapMinSize=0x1000;
const TInt KHeapMaxSize=0x1000;

// Create a new thread
RThread *spinthread = new RThread;

//Periadic Bip
CPeriodic*	Timer;

TInt SMPSpinThread(TAny*);


struct TThread
	{
	RThread thread;
	TDesC threadName;
	TInt threadPriority;
	TInt cpuAffinity;
	TInt loopCount;
	TInt endLoopDelay;
	TBool fixedCPU;
	TBool endFlag;
	};

TThread ThreadTable[] =
	{
		{ RThread(), _L("Thread1"),  EPriorityAbsoluteHigh,     0, 1000, 100, EFalse,    EFalse}, 	
		{ RThread(), _L("Thread2"),  EPriorityAbsoluteHigh,     0, 1000, 100, EFalse,    EFalse}, 	 
	};

enum 
	{
	KThreads = (TInt)(sizeof(ThreadTable) / sizeof(TThread))
};

void ShowHelp()
	{
	PRINT(_L("***************************************\n"));
	PRINT(_L("The following are available commands\n"));
	PRINT(_L("Esc     to exit\n"));
	PRINT(_L("***************************************\n"));
	}

TInt NumberOfCpus()
	{
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);
	test(r>0);
	return r;
	}


void ParseCommandLine ()
	{
	TBuf<64> c;
	
	User::CommandLine(c);
	c.LowerCase();

	if (c != KNullDesC)
		{
		TLex lex(c);
		TPtrC token;

		while (token.Set(lex.NextToken()), token != KNullDesC)
			{
			if (token.Mid(0) == _L("quiet"))
				{
				gQuiet = ETrue;
				continue;
				}

			if (token.Mid(0) == _L("verbose"))
				{
				gQuiet = EFalse;
				continue;
				}
			}
		}
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

TInt E32Main()
	{
	test.Title();
	__UHEAP_MARK;
	test.Start(_L("SMP Soak Test"));

	test.Next(_L("Load device driver"));
	TInt r = User::LoadLogicalDevice(_L("d_smpsoak.ldd"));
	if (r == KErrNotFound)
		{
		test.Printf(_L("Test not supported on this platform because the D_SMPSOAK.LDD Driver is Not Present\n"));
		test.End();
		return 0;
		}

	test.Next(_L("Calling rt.Open"));
	RSMPSoak rt;
	r = rt.Open();
	if (r!=KErrNone)
		{
		test.Printf(_L("Error- Couldn't able to open soak driver:%d"),r);
		return r;
		}
	test.Next(_L("rt.Open called"));

    spinthread->Create(_L("SMPSpinThread"), SMPSpinThread, KDefaultStackSize, KHeapMinSize, KHeapMaxSize, &rt);
	DEBUG_PRINT((_L("SMPSoak Thread is %x\n"), spinthread));

	spinthread->SetPriority(EPriorityAbsoluteLow);

	spinthread->Resume();
	
	ParseCommandLine();
	
	CActiveScheduler* myScheduler = new (ELeave) CActiveScheduler();
	test(myScheduler !=NULL);
	CActiveScheduler::Install(myScheduler);

	CPeriodic* theTimer=NULL;
	TRAPD(ret,theTimer=CActiveConsole::TimerL())
	test_KErrNone(ret);
	theTimer->Start(0,KTimerPeriod,TCallBack(CActiveConsole::Callback));

	CActiveConsole* myActiveConsole = new CActiveConsole();
	test(myActiveConsole !=NULL);
	myActiveConsole->GetCharacter();

	CActiveScheduler::Start();

	delete theTimer;

	 __UHEAP_MARKEND;

	test.End();

	return 0;
	}

TInt SMPSpinThread(TAny* rt)
{
	TInt startCpu = 0x00;
	TInt endCpu = 0x00;
	RTimer timer;
	test(timer.CreateLocal()==KErrNone);
	TRequestStatus s;

	RSMPSoak* pMyDriver = (RSMPSoak*)rt;
	RTest test(_L("SMPSpinThread"));
	test.Title();

	FOREVER
	{
		startCpu = pMyDriver->TryControl(RSMPSoak::KGETCURRENTCPU, 0);
		
		timer.After(s, 250000);			
		User::WaitForRequest(s);
		test(s==KErrNone);
		DEBUG_PRINT((_L("+")));

		endCpu = pMyDriver->TryControl(RSMPSoak::KGETCURRENTCPU, 0);
		
		if(startCpu != endCpu)
		{
			DEBUG_PRINT((_L("\r\nSMPSoakSpin app:- Thread moved from cpu %d to cpu %d ************\n"), startCpu, endCpu));
		}
		if (gAbort)
			break;
	}
	
	timer.Cancel();
	DEBUG_PRINT((_L("MyTimer.Cancel() called\n")));
	return 0x00;	
}


