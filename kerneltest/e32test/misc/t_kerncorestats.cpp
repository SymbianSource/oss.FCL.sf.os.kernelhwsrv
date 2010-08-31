// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\t_kerncorestas.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include <u32hal.h>
#include <e32property.h>

#include "d_testkerncorestats.h"

LOCAL_D RTest test(_L("t_KernCoreStats"));


// Kernel side stats (also used when obtaining stats from Kernel)
const TInt KStatsCoreNumTimesInIdle	 = 0x0001;
const TInt KStatsCoreTotalTimeInIdle	 = 0x0002;
const TInt KStatsTimeCrossIdleAndActive	 = 0x0004;
const TInt KStatsReadyStateChanges	 = 0x0008;
const TInt KStatsNumTimeSliceExpire	 = 0x0010;
const TInt KStatsNumEvents		 = 0x0020;


_LIT(KLddName, "d_testkerncorestats");
static RTestKernCoreStats gChan;
TInt gCores;
TInt gDelay = 500;
TInt gThreads=0;

void DisplayBuf(TInt aMode, TAny* aBuffer)
	{
	TUint* pVal = (TUint*) aBuffer;
	test.Printf(_L("\nKernel Stats\n============\n"));

	TInt core;
	TUint TTI=0;

	if (aMode & KStatsCoreTotalTimeInIdle)
		{
		test.Printf(_L("Time in Idle thread:"));	
		TTI=0;
		for (core=0; core<gCores; core++)
			{
			test.Console()->SetPos(22);
			test.Printf(_L("%d: %10d \n"),core,*pVal);
			TTI+=*pVal;
			pVal++;
			}

		test.Console()->SetPos(22);
		test.Printf(_L("Total:%10d \n\n"),TTI);
		}


	if (aMode & KStatsTimeCrossIdleAndActive)
		{
		TUint total=0;
		TUint idletotal=0;
		
		TUint* pSavedPlace = pVal;

		for (core=0;  core<(gCores+1); core++)
			{
			total+=*pVal;
			idletotal+= *pVal*core;
			pVal++;		
			}
		pVal = pSavedPlace;

		TInt percent;
		test.Printf(_L("Time in permutations: "));
		for (core=0;  core<(gCores+1); core++)
			{
			test.Console()->SetPos(22);
			percent = (total>0)?*pVal*100/total:0;
			test.Printf(_L("%d: %10d (%3d%%)\n"),core,*pVal, percent);
			pVal++;		
			}




		test.Console()->SetPos(22);
		test.Printf(_L("Total Time :%10d        "),total);
		if ((total- (TUint) gDelay > 100000) || ((TUint) gDelay>total))
			test.Printf(_L("FAIL!\n"));
		else
			test.Printf(_L("PASS \n"));
		

		test.Console()->SetPos(22);
		percent = (total>0)?idletotal*25/total:0;
		test.Printf(_L("Total Idle :%10d (%3d%%) "), idletotal, percent);
		if (aMode & KStatsCoreTotalTimeInIdle)
			{
			if (idletotal==TTI)
				test.Printf(_L("PASS "));
			else
				test.Printf(_L("FAIL!"));
			}
		test.Printf(_L("\n\n"));
		}

	if (aMode & KStatsCoreNumTimesInIdle)
		{
		test.Printf(_L("Numer times in idle :"));
		for (core=0; core<gCores; core++)
			{
			test.Console()->SetPos(22);
			test.Printf(_L(" %d: %6d \n"),core,*pVal);
			pVal++;
			}
		test.Printf(_L(" \n"));	
		}

	if (aMode & KStatsNumEvents)
		{	
		test.Printf(_L("Number of Events    : %d   \n"),*pVal);
		pVal++;
		}



	if (aMode & KStatsReadyStateChanges)
	{	
		test.Printf(_L("Threads made Ready  : %d   \n"),*pVal);
		pVal++;
		test.Printf(_L("Threads made unready: %d   \n"),*pVal);
		pVal++;
	}

	if (aMode & KStatsNumTimeSliceExpire)
		test.Printf(_L("Time slices expired : %d   \n"),*pVal);

	test.Printf(_L("BG threads: %d, delay %d \n"),gThreads,gDelay);

	}
	
void InitBuff(TAny* aBuffer, TInt aSize)
	{
	TInt* pos = (TInt*) aBuffer;

	TInt i;
	for (i=0; i<aSize; i+=4)
		{
		*pos = -1;
		pos++;
		}
	}

RProperty gThreadsProperty;


LOCAL_C TInt BusyThread(TAny* aThreadNo)
	{
	TInt threadNo= (TInt) aThreadNo;
	TInt threadsNeeded;
	TInt dummy=0;
	do
		{
		TInt count;
		for (count=0; count<10000; count++)
			{
			dummy=(dummy+count/3)/7;
			}

		gThreadsProperty.Get(threadsNeeded);
		}
	while (threadNo<=threadsNeeded);
	return(KErrNone);
	}



const TInt KMaxThreads=16;
RThread gThreadlist[KMaxThreads];
TRequestStatus gThreadlist_stat[KMaxThreads];

void AddThread()
	{
	if (gThreads==KMaxThreads)
		return;
	gThreads+=1;
	test_KErrNone(gThreadsProperty.Set(gThreads));

	test_KErrNone(gThreadlist[gThreads-1].Create(_L(""),BusyThread,KDefaultStackSize,NULL,(TAny*) gThreads));
	gThreadlist[gThreads-1].Logon(gThreadlist_stat[gThreads-1]);
	gThreadlist[gThreads-1].SetPriority(EPriorityLess);
	gThreadlist[gThreads-1].Resume();
	}
void DecThread()
	{
	if (gThreads==0)
		return;
	gThreads-=1;
	test_KErrNone(gThreadsProperty.Set(gThreads));

	User::WaitForRequest(gThreadlist_stat[gThreads]);
	gThreadlist[gThreads].Close();
	}

void RunTest(TInt aMode)
	{
	TAny* buffer;
	TInt r;
	gCores = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);
	test_Compare(gCores, >, 0);

	test.Next(_L("Load test kerncorestats LDD"));
	r = User::LoadLogicalDevice(KLddName);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);  
	test.Next(_L("Open test kerncorestats LDD"));

	test_KErrNone(gChan.Open());
	
	test.Next(_L("Call configure"));

	r = gChan.Configure(aMode);
	if (r==KErrInUse)
		{
		test.Printf(_L("Cannot configure KernCoreStats!!\nThis could be becouse you have already run this test since the last reboot (You can only call it once.) or that you are running a power manager that has already configured this."));
		test.Printf(_L("Any such power manager must be disabled in order to run this test/util.\n\nPress a key.\n"));
		test.Getch();
		}
	test_KErrNone(r);
		

	test.Next(_L("Call DumpInfo"));

	r = gChan.DumpInfo();
	test_NotNegative(r);
	TInt size = r;

	RProcess thisProcess;

	test_KErrNone(RProperty::Define(thisProcess.SecureId(), 0, RProperty::EInt));

	test_KErrNone(gThreadsProperty.Attach(thisProcess.SecureId(), 0));
	test_KErrNone(gThreadsProperty.Set(0));


	test.Printf(_L("Press a key to show output\n"));
	test.Getch();


	test.Next(_L("Start showing output."));
	test.Printf(_L("Press ESC to stop. u/k threads h/m delay.\n"));



	buffer=User::Alloc(size);
	test_NotNull(buffer);

	CConsoleBase* con = test.Console();
	TInt y = con->WhereY();
	TRequestStatus s;
	TInt keycode;

	do 
		{
		con->Read(s);
		while (s==KRequestPending)
			{
			con->SetPos(0,y);
			InitBuff(buffer,size);
			test_KErrNone(gChan.GetStats(buffer));
		
			DisplayBuf(aMode,buffer);
			User::After(gDelay*1000);
			};

		keycode=con->KeyCode();
		switch ((TChar) keycode)
			{
			case 'u': AddThread();
			break;
			case 'k': DecThread();
			break;
			case 'h': gDelay+=100;
			break;
			case 'm': gDelay=(gDelay>99)?gDelay-100:gDelay;
			break;
			}

		}
	while (keycode!=27);

	test.Next(_L("Close test kerncorestats LDD"));
	while (gThreads>0)
		DecThread();

	gThreadsProperty.Close();
	RProperty::Delete(0);
	gChan.Close();

	test_KErrNone( User::FreeLogicalDevice(KTestKernCoreStatsName) );
	User::Free(buffer);
	}

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

//
// ParseCommandLine reads the arguments and sets globals accordingly.
//

TInt ParseCommandLine()
	{
	TInt mode=0;
	TInt moderead;
	TBuf<32> args;
	User::CommandLine(args);
	TLex lex(args);
	TInt err=KErrNone;
	FOREVER
		{
		TPtrC token=lex.NextToken();
		TPtrC subtoken(_L(""));

		if(token.Length()!=0)
			{
			if ((token==_L("help")) || (token==_L("-h")) || (token==_L("/h")) || (token==_L("-?")) || (token==_L("/?")))
				{
				test.Printf(_L("\nThis tests kerncorestats - but not automatically.  It can only be run *ONCE* per boot.\n\n"));
				test.Printf(_L("\n -h : Help.\n -m mode number, which specifies which stats are collected."));
				err=KErrCancel;
				}
			
			else
				if (token==_L("-m"))
					{
					subtoken.Set(lex.NextToken());

					TLex lexv(subtoken);

					if(subtoken.Length()==0)
						{
						subtoken.Set(_L("<NOTHING>"));
						err = KErrArgument;
						}
					else if (lexv.Val(moderead)!=KErrNone)
						err = KErrArgument;
					else
						mode|=moderead;

					} // endif -m
			else
				{
				err = KErrArgument;
				} // endif token 
			}
		else
			break;
		
		if (err!=KErrNone)
			{
			if (err==KErrArgument)
				test.Printf(_L("\nUnknown argument '%S%s%S'\n"), &token, (subtoken.Length()==0)?"":" ", &subtoken);
			test.Printf(_L("\nUsage:  t_kerncorestats -m <mode>\n\n"));
			test.Getch();
			return err;
			}
		}

	if (mode==0)
		{
		mode=63;
		}

	return mode;
	}


GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("KernCoreStats manual test"));
	TInt mode = ParseCommandLine();
	if (mode>0)
		RunTest(mode);
   	test.End();

	return KErrNone;  
	}

