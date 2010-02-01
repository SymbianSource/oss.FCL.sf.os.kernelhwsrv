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
// e32test\debug\t_codemodifier.cpp
// Overview:
// Exercises Kernel's Code Modifier. This is common use case for run-mode debuggers.
// API Information:
// DebugSupport::InitialiseCodeModifier
// DebugSupport::CloseCodeModifier
// DebugSupport::ModifyCode
// DebugSupport::RestoreCode
// Kern::ThreadRawRead
// Kern::ThreadRawWrite
// Details:
// -Three applications are running in the test:
// - t_codemodifier.exe client
// - t_codemodifier.exe server (XIP code)
// - t_codemodifier2.exe client (non-XIP code)
// -Test1 -Checks that the TestFunc (that will be altered by breakpoints) is really deadly if un-altered.
// -Test2 -Checks Code Modifier if data segment address is passed to set breakpoint.	
// -Test3 -Checks Code Modifier if invalid address is passed to set breakpoint.Kern::ThreadRawRead\Write is also checked.	
// -Test4 -Replaces BRK instruction in TestFunc with NOP using 1.2 and 4 bytes long breakpoints. Executes the 
// function in the servers.
// -Test5 -Repeats Test4 (for XIP server only) with previously shedowed TestFunc
// -Test6 -Tests scenario when a process terminates while there are still associated breakpoints.
// -Test7 -Tests out-of-breakpoints scenario
// -Test8 -Tests breakpoint-already-exists scenario
// -Test9 -Tests CodeModifier closing when there are still active breakpoints.	Breakpoints in this test occupies more then
// one shadowed page.
// -Test10-A random stress test. Sets/Clears random brekpoints in CodeArea of the both servers. Then replaces
// all BRKs with NOPs in CodeArea and executes them.
// -Test11-Checks that overlaping breakpoints are rejected.
// Platforms/Drives/Compatibility:
// Hardware (Automatic). Not supported on emulator.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32ldr.h>
#include <e32ldr_private.h>
#include <f32dbg.h>
#include "d_codemodifier.h"
#include "../misc/prbs.h"

LOCAL_D RTest test(_L("T_CODEMODIFIER"));
_LIT(KServerXIP,"ServerXIP");
_LIT(KServerNONXIP,"ServerNONXIP");
_LIT(KAppXIP,"t_codemodifier.exe");
_LIT(KAppNONXIP,"t_codemodifier2.exe");

extern void TestFunc();
extern void CodeArea();

//------------client globals---------------------
RCodeModifierDevice Device;

/**	These addresses/names is all the client needs to test a server.
	There are XIP and non-XIP server in the test*/
struct SServerData
	{
	TInt  		 iThreadId;
	TUint 		 iVarAddr;
	TUint 		 iFuncAddr;
	TUint 		 iInvalidAddr;
	TUint 		 iCodeAreaAddr;
	const TDesC* iAppName;
	const TDesC* iServerName;
	RProcess 	 iProcess;
	} ServerData[2];

/**Will hold assembler instructions that make TestFunc.*/
struct STestFunc
	{
	TInt iBRK;
	TInt iNOP;
	TInt iRET;
	} TestFuncCode;

/**ServerData[0] is about XIP server, ServerData[1] is non-XIP server*/
enum TWhichServer
	{
	EXip = 0,
	ENonxip = 1
	};

/**Server*/
class CCodeModifierServer : public CServer2
	{
public:
	static CCodeModifierServer* New(TInt aPriority);
private:
	CCodeModifierServer(TInt aPriority) : CServer2(aPriority){}
	CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const;
public:
	TInt iTestVar;
	RChunk chunk;
	};

CCodeModifierServer* CCodeModifierServer::New(TInt aPriority)
	{
	return new CCodeModifierServer(aPriority);
	}

/**Server side session*/
class CCodeModifierSession : public CSession2
	{
public:
	enum TCommand {EStop, EGetServerInfo, ERunTestFunc, ERunCodeAreaFunc};
private:
	void ServiceL(const RMessage2& aMessage);
	};

/*Client-side session*/
class RCodeModifierSession : private RSessionBase
	{
public:
	/**Updates ServerData[aServer] with data from the server*/
	static TInt GetServerInfo(TInt aServer)
		{
		TPtr8 ptr((TUint8*)&ServerData[aServer].iThreadId, 5*sizeof(TInt),5*sizeof(TInt));
		TIpcArgs args(&ptr);
		return Control(CCodeModifierSession::EGetServerInfo,aServer,args);
		};
	/**Stops the server*/	
	static TInt Stop(TInt aServer)
		{TIpcArgs args;return Control(CCodeModifierSession::EStop,aServer, args);};
	/**Issues the command to server to run TestFunc*/
	static TInt RunTestFunc(TInt aServer)
		{TIpcArgs args;return Control(CCodeModifierSession::ERunTestFunc,aServer,args);};
	/**Issues the command to server to run CodeArea*/
	static TInt RunCodeAreaFunc(TInt aServer)
		{TIpcArgs args;return Control(CCodeModifierSession::ERunCodeAreaFunc,aServer,args);};
private:
	/*Executes a synchronius client-server request*/
	static TInt Control(CCodeModifierSession::TCommand aRequest, TInt aServer, TIpcArgs& aArgs)
		{
		RCodeModifierSession p;
		TInt r = p.CreateSession(*ServerData[aServer].iServerName, TVersion(), 0);
		if (r == KErrNone)
			p.SendReceive(aRequest, aArgs);
		p.Close();
		return r;
		};
	};

//------------server globals---------------------
CCodeModifierServer* CodeModifierServer;

/**Creates a new client for this server.*/
CSession2* CCodeModifierServer::NewSessionL(const TVersion&, const RMessage2&) const
	{
	return new(ELeave) CCodeModifierSession();
	}

/**Session entry point on the server side.*/
void CCodeModifierSession::ServiceL(const RMessage2& aMessage)
	{
	TInt r=KErrNone;
	switch (aMessage.Function())
		{
	case EGetServerInfo:		//Pass threadId and addresses to the client
		{
		struct SInfo
			{
			TInt  iThreadId;
			TUint iVarAddr;
			TUint iFuncAddr;
			TUint iInvalidAddr;
			TUint iCodeAreaAddr;
			} info;
		RThread thread;
		info.iThreadId =    (TInt) thread.Id();
		info.iVarAddr =     (TUint) &CodeModifierServer->iTestVar;
		info.iInvalidAddr = (TUint)CodeModifierServer->chunk.Base()+0x1000;
		info.iFuncAddr =    (TUint)&TestFunc;
		info.iCodeAreaAddr =(TUint)&CodeArea;
		TPtrC8 ptr((TUint8*)&info, sizeof(SInfo));
		r=aMessage.Write(0,ptr);
		}
		break;

	case ERunTestFunc:			//Execute TestFunc
		TestFunc();
		break;

	case ERunCodeAreaFunc:			//Execute CodeArea
		CodeArea();
		break;

	case EStop:					//This will stop the server thread.
		CActiveScheduler::Stop();
		break;

	default:
		r=KErrNotSupported;
		}
	aMessage.Complete(r);
	}

/**Server application entry point*/
LOCAL_C TInt ServerEntryPoint(TInt aServer)
	{
	TInt r=0;
	__UHEAP_MARK;
	
	CActiveScheduler *pR=new CActiveScheduler;
	if (!pR)
		User::Panic(_L("SVR:Could't create Active Scheduler\n"), KErrNoMemory);
	CActiveScheduler::Install(pR);

	CodeModifierServer = CCodeModifierServer::New(0);
	if(!CodeModifierServer)
		{
		delete pR;
		User::Panic(_L("SVR:Create svr error\n"), KErrNoMemory);
		}

	//Create a chunk with a hole between addresses 0x1000 & 0x2000
	r=CodeModifierServer->chunk.CreateDisconnectedLocal(0,0,0x200000);
	test_KErrNone(r);
	r=CodeModifierServer->chunk.Commit(0,0x1000);
	test_KErrNone(r);
	r=CodeModifierServer->chunk.Commit(0x2000,0x1000);
	test_KErrNone(r);

	//We decide here which server to start.
	if (aServer==0)
		r=CodeModifierServer->Start(KServerXIP);
	else
		r=CodeModifierServer->Start(KServerNONXIP);
		
	if (r!=KErrNone)
		{
		delete CodeModifierServer;
		delete pR;
		User::Panic(_L("SVR:Error starting server\n"), r);
		}
	RProcess::Rendezvous(KErrNone);
	CActiveScheduler::Start();

	//We come here on CActiveScheduler::Stop()
	delete CodeModifierServer;
	delete pR;
	__UHEAP_MARKEND;
	return(KErrNone);
	}

//Initializes the globals and switch off lazy (un)loader. Called just once at the start.
void UpdateClientsGlobals()
	{
	TInt* ptr = (TInt*)TestFunc;
	TestFuncCode.iBRK = *(ptr++);
	TestFuncCode.iNOP = *(ptr++);
	TestFuncCode.iRET = *(ptr++);
	test.Printf(_L("UpdateClientsGlobals BRK:%x NOP:%x RET:%x\n"),TestFuncCode.iBRK,TestFuncCode.iNOP,TestFuncCode.iRET);

	ServerData[0].iAppName=&KAppXIP;
	ServerData[1].iAppName=&KAppNONXIP;
	ServerData[0].iServerName=&KServerXIP;
	ServerData[1].iServerName=&KServerNONXIP;

	//Turn off lazy dll (un)loader.
	//If we don't this, the second run of the test (within 2 minutes of the first one) would fail.
	RLoader l;
	test_KErrNone(l.Connect());
	test_KErrNone(l.CancelLazyDllUnload());
	l.Close();
	}
	
//Starts the server (0-XIP 1-NONXIP server)
//Obtains data from the server
//Updates the driver with the threadID of the server
void StartAndUpdate(TInt aServer)
	{
	TRequestStatus status;
	RProcess& p = ServerData[aServer].iProcess;
	test.Printf(_L("StartAndUpdate %d\n"),aServer);
	//Starts the server
	TInt r = p.Create(*ServerData[aServer].iAppName,*ServerData[aServer].iServerName);
	test_KErrNone(r);
	p.Rendezvous(status);
	p.Resume();
	User::WaitForRequest(status);
	test.Printf(_L("%d returned\n"),status.Int());
	test_KErrNone(status.Int());
	//Get threadId and addresses from the server
	test_KErrNone(RCodeModifierSession::GetServerInfo(aServer));
	SServerData& s = ServerData[aServer];
	test.Printf(_L("ServerData:TId:%x VA:%x FA:%x IA:%x CA:%x \n"),s.iThreadId,s.iVarAddr,s.iFuncAddr,s.iInvalidAddr,s.iCodeAreaAddr);
	//Update threadID of the server in device driver
	test_KErrNone(Device.ThreadId(aServer, s.iThreadId));
	}

//Kills the server(by forcing to execute TestFunc), then restarts it.
void KillAndRestart(TInt aServer)
	{
	test.Printf(_L("KillAndRestart %d\n"),aServer);
	TInt r=RCodeModifierSession::RunTestFunc(aServer);
	test.Printf(_L("%d returned\n"),r);

	test.Printf(_L("Check the server died\n"));
	r = RCodeModifierSession::GetServerInfo(aServer);
	test.Printf(_L("%d returned\n"),r);
	test(r!=KErrNone);
	
	StartAndUpdate(aServer);
	}

//Terminates the server
void TerminateServer(TInt aServer)
	{
	TRequestStatus status;
	RProcess& p = ServerData[aServer].iProcess;

	test.Printf(_L("TerminateServer %d\n"),aServer);
	test_KErrNone(RCodeModifierSession::Stop(aServer));
	p.Logon(status);
	User::WaitForRequest(status);
	test_Equal(EExitKill, p.ExitType());
	CLOSE_AND_WAIT(p);
	}

//Starts LDD
void StartDriver()
	{
	test.Printf(_L("StartDriver\n"));
	TInt r = User::LoadLogicalDevice(KCodeModifierName);
	test_Value(r, r==KErrNone || r==KErrAlreadyExists);
	if((r = Device.Open())!=KErrNone)	
		{
		User::FreeLogicalDevice(KCodeModifierName);
		test.Printf(_L("Could not open LDD"));
		test(0);
		}
	}

//Unloads LDD
void StopDriver()
	{
	test.Printf(_L("StopDriver\n"));
	Device.Close();
	User::FreeLogicalDevice(KCodeModifierName);
	}

//Checks that TestFunc in servers is really deadly if we do not alter the code
void Test1()
	{
	test.Printf(_L("Test1\n"));
	KillAndRestart(EXip);
	}

//Passing data segment address to set breakpoint.
//The actual behaviour depends of the memory model
//ARMv5: KErrBadDescriptor (-38) is returned
//ARMv6: Will return 0
//We have to make sure here that nothing panics or take any nasty behaviour.
void Test2(TInt aServer)
	{
	TInt val;
	test.Printf(_L("Test2 %d\n"), aServer);
	test_KErrNone(Device.InitialiseCodeModifier(10));

	//Write/read data through ThreadRowWrite/Read
	test_KErrNone(Device.WriteWord(aServer, ServerData[aServer].iVarAddr,/*value*/1));
	test_KErrNone(Device.ReadWord(aServer, ServerData[aServer].iVarAddr,&val));
	test_Equal(1, val);
		
	//Set breakpoint
	TInt r=Device.WriteCode(aServer, ServerData[aServer].iVarAddr,/*value*/5, /*size*/4);
	test.Printf(_L("returns %d\n"),r);

	test_KErrNone(Device.CloseCodeModifier());
	}

//Passing invalid address to set breakpoint.
void Test3(TInt aServer)
	{
	TInt val;
	test.Printf(_L("Test3 %d\n"), aServer);
	test_KErrNone(Device.InitialiseCodeModifier(10));

	//Write/read by ThreadRowWrite/Read
	test_Equal(KErrBadDescriptor, Device.WriteWord(aServer, ServerData[aServer].iInvalidAddr,/*value*/1));
	test_Equal(KErrBadDescriptor, Device.ReadWord(aServer, ServerData[aServer].iInvalidAddr,&val));
		
	//Set breakpoints
	test_Equal(KErrBadDescriptor, Device.WriteCode(aServer, ServerData[aServer].iInvalidAddr,/*value*/5, /*size*/1));
	test_Equal(KErrBadDescriptor, Device.WriteCode(aServer, ServerData[aServer].iInvalidAddr,/*value*/5, /*size*/2));
	test_Equal(KErrBadDescriptor, Device.WriteCode(aServer, ServerData[aServer].iInvalidAddr,/*value*/5, /*size*/4));

	test_KErrNone(Device.CloseCodeModifier());
	}

//Replace BRK in TestFunc in server using 1,2 and 4 long breakpoints.
//Check the content of test func.
//Execute Test Func
void Test4(TInt aServer)
	{
	TInt var;
	test.Printf(_L("Test4 %d\n"), aServer);

	//Try to write code segment throught Kern::ThreadRowWrite
	test_Equal(KErrBadDescriptor, Device.WriteWord(aServer, ServerData[aServer].iFuncAddr,/*value*/1));

	test_KErrNone(Device.InitialiseCodeModifier(10));

	test.Printf(_L("Replace byte 3 of the 1st instruction\n"));
	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iFuncAddr+3,TestFuncCode.iNOP>>24, /*size*/1));
	test_KErrNone(Device.ReadWord(aServer,ServerData[aServer].iFuncAddr, &var));
	test.Printf(_L("%xH returned\n"),var);
	test((TUint)var == ((TestFuncCode.iBRK & 0xffffff) | (TestFuncCode.iNOP & 0xff000000)));

	test.Printf(_L("Replace byte 2 of the 1st instruction\n"));
	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iFuncAddr+2,TestFuncCode.iNOP>>16, /*size*/1));
	test_KErrNone(Device.ReadWord(aServer,ServerData[aServer].iFuncAddr, &var));
	test.Printf(_L("%xH returned\n"),var);
	test((TUint)var == ((TestFuncCode.iBRK & 0xffff) | (TestFuncCode.iNOP & 0xffff0000)));

	test.Printf(_L("Replace bytes 0 & 1 of the 1st instruction\n"));
	var = TestFuncCode.iNOP | 0xff0000; //byte 3 is messed up - but it won't be writen into code bacause iSize is 2
	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iFuncAddr,var, /*size*/2));
	test_KErrNone(Device.ReadWord(aServer,ServerData[aServer].iFuncAddr, &var));
	test.Printf(_L("%xH returned\n"),var);
	test_Equal(TestFuncCode.iNOP, var);

	//We have replaced BRK with NOP. It should be safe now for the server to execute TestFunc.
	test.Printf(_L("Run TestFunc in server and check the server is still alive\n"));
	test_KErrNone(RCodeModifierSession::RunTestFunc(aServer));
	test_KErrNone(RCodeModifierSession::GetServerInfo(aServer));//Any call will work here

	test.Printf(_L("Revert bytes 0 & 1\n"));
	test_KErrNone(Device.RestoreCode(aServer, ServerData[aServer].iFuncAddr));
	test_KErrNone(Device.ReadWord(aServer, ServerData[aServer].iFuncAddr, &var));
	test.Printf(_L("%xH returned\n"),var);
	test((TUint)var == ((TestFuncCode.iBRK & 0xffff) | (TestFuncCode.iNOP & 0xffff0000)));

	test.Printf(_L("Revert byte 2\n"));
	test_KErrNone(Device.RestoreCode(aServer, ServerData[aServer].iFuncAddr+2));
	test_KErrNone(Device.ReadWord(aServer, ServerData[aServer].iFuncAddr, &var));
	test.Printf(_L("%xH returned\n"),var);
	test((TUint)var == ((TestFuncCode.iBRK & 0xffffff) | (TestFuncCode.iNOP & 0xff000000)));

	test.Printf(_L("Revert byte 3\n"));
	test_KErrNone(Device.RestoreCode(aServer, ServerData[aServer].iFuncAddr+3));
	test_KErrNone(Device.ReadWord(aServer, ServerData[aServer].iFuncAddr, &var));
	test.Printf(_L("%xH returned\n"),var);
	test(var == TestFuncCode.iBRK);

	test.Printf(_L("Replace the 1st instruction with the 2nd one in TestFunc\n"));
	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iFuncAddr,TestFuncCode.iNOP, /*size*/4));
	test_KErrNone(Device.ReadWord(aServer,ServerData[aServer].iFuncAddr, &var));
	test.Printf(_L("%xH returned\n"),var);
	test(var == TestFuncCode.iNOP);

	//We have replaced BRK with NOP. It should be safe now for the server to execute TestFunc.
	test.Printf(_L("Run TestFunc in server and check the server is still alive\n"));
	test_KErrNone(RCodeModifierSession::RunTestFunc(aServer));
	test_KErrNone(RCodeModifierSession::GetServerInfo(aServer));//Any call will work here

	test_KErrNone(Device.CloseCodeModifier());
	}

//Executes Test4 but with previously shadowed page
void Test5(TInt aServer)
	{
	test.Printf(_L("Test5 %d\n"), aServer);

	test_KErrNone(Device.AllocShadowPage(ServerData[aServer].iFuncAddr));
	Test4(aServer);	
	test_KErrNone(Device.FreeShadowPage(ServerData[aServer].iFuncAddr));
	}

//Tests scenario when a process terminates while there are still associated breakpoints.
void Test6(TInt aServer)
	{
	TInt var;
	test.Printf(_L("Test6 %d\n"), aServer);

	test_KErrNone(Device.InitialiseCodeModifier(10));

	test.Printf(_L("Replace the 1st instruction (BRK) with the 2nd one (NOP) in TestFunc\n"));
	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iFuncAddr,TestFuncCode.iNOP, /*size*/4));
	test_KErrNone(Device.ReadWord(aServer,ServerData[aServer].iFuncAddr, &var));
	test.Printf(_L("%xH returned\n"),var);
	test(var == TestFuncCode.iNOP);

	TerminateServer(aServer);
	//After application has stopped, Kernel should clean the breakpoint associated to the server's process....
	
	StartAndUpdate(aServer);
	KillAndRestart(aServer);//... and TestFunct must be deadly again.

	test_KErrNone(Device.CloseCodeModifier());
	}

//Tests out-of-breakpoints scenario
void Test7(TInt aServer)
	{
	test.Printf(_L("Test7 %d\n"), aServer);
	test_KErrNone(Device.InitialiseCodeModifier(1));

	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr,TestFuncCode.iNOP, /*size*/4));
	test_Equal(KErrNoMemory, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr+4,TestFuncCode.iNOP, /*size*/4));

	test_KErrNone(Device.CloseCodeModifier());
	}

//Tests breakpoint-already-exists scenario
void Test8(TInt aServer)
	{
	test.Printf(_L("Test8 %d\n"), aServer);
	test_KErrNone(Device.InitialiseCodeModifier(1));

	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr,TestFuncCode.iNOP, /*size*/4));
	test_Equal(KErrAlreadyExists, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr,TestFuncCode.iNOP, /*size*/4));

	test_KErrNone(Device.CloseCodeModifier());
	}

//Tests CodeModifier closing when there are still breakpoints.
//Breakpoints in this test occupies more then one shadowed page.
void Test9()
	{
	TInt var;
	test.Printf(_L("Test9\n"));
	test_KErrNone(Device.InitialiseCodeModifier(10));

	//Put NOPs at the beginning of the code area
	test_KErrNone(Device.WriteCode(EXip, ServerData[EXip].iCodeAreaAddr,TestFuncCode.iNOP, /*size*/4));
	test_KErrNone(Device.WriteCode(ENonxip, ServerData[ENonxip].iCodeAreaAddr,TestFuncCode.iNOP, /*size*/4));

	//Put NOPs at the end of the code area (there are 1024 BRK instructions in CodeArea
	test_KErrNone(Device.WriteCode(EXip, ServerData[EXip].iCodeAreaAddr+1024*sizeof(TInt),TestFuncCode.iNOP, /*size*/4));
	test_KErrNone(Device.WriteCode(ENonxip, ServerData[ENonxip].iCodeAreaAddr+1024*sizeof(TInt),TestFuncCode.iNOP, /*size*/4));

	//Check NOPs are there
	test_KErrNone(Device.ReadWord(EXip,ServerData[EXip].iCodeAreaAddr, &var));
	test_Equal(TestFuncCode.iNOP, var);
	test_KErrNone(Device.ReadWord(ENonxip,ServerData[ENonxip].iCodeAreaAddr, &var));
	test_Equal(TestFuncCode.iNOP, var);
	test_KErrNone(Device.ReadWord(EXip,ServerData[EXip].iCodeAreaAddr+1024*sizeof(TInt), &var));
	test_Equal(TestFuncCode.iNOP, var);
	test_KErrNone(Device.ReadWord(ENonxip,ServerData[ENonxip].iCodeAreaAddr+1024*sizeof(TInt), &var));
	test_Equal(TestFuncCode.iNOP, var);

	//Close Code Modifier. It should revert the changes in the code.
	test_KErrNone(Device.CloseCodeModifier());

	//Check BRKs are back
	test_KErrNone(Device.ReadWord(EXip,ServerData[EXip].iCodeAreaAddr, &var));
	test_Equal(TestFuncCode.iBRK, var);
	test_KErrNone(Device.ReadWord(ENonxip,ServerData[ENonxip].iCodeAreaAddr, &var));
	test_Equal(TestFuncCode.iBRK, var);
	test_KErrNone(Device.ReadWord(EXip,ServerData[EXip].iCodeAreaAddr+1023*sizeof(TInt), &var));
	test_Equal(TestFuncCode.iBRK, var);
	test_KErrNone(Device.ReadWord(ENonxip,ServerData[ENonxip].iCodeAreaAddr+1023*sizeof(TInt), &var));
	test_Equal(TestFuncCode.iBRK, var);
	}



//Used in test 10 to keep the list of breakpoints
class TBrks:public CBase //derived from CBase as we need data initialized to 0
	{
public:	
	TInt iCounter;				//Counts the number of the active brakpoints
	TInt8 iBreakpoint[1025][2];		//0 - no breakpoint, 1-breakpoint set in CodeArea of XIP Server & NON-XIP server
	};

//Performs a random stress test on breakpoint pool.
//There are 1025*2 words in CodeArea in xip and non-xip server.
//A word is randomly picked up to set or clear 4 bytes long breakpoint.
void Test10()
	{
	TInt i,index,whichServer,var;
	TBrks* brks = new TBrks;
	test((TInt)brks);//fail if no memory
	TUint iSeed[2];
	iSeed[0]=User::TickCount();
	iSeed[1]=0;
	test.Printf(_L("Test10 iSeed=%x\n"), iSeed[0]);

	test_KErrNone(Device.InitialiseCodeModifier(2050));//enought to replace all BRK instructions in CodeArea with NOPs in both servers

	for (i=0; i<1000;i++)
		{
		index=Random(iSeed)%2050;
		whichServer = index>1024 ? 1 : 0;
		if (index >1024)
			index-=1025;
		
		TInt8& brk = brks->iBreakpoint[index][whichServer];
		 if (brk)
		 	{//Remove breakpoint
		 	brk = 0;
			test_KErrNone(Device.RestoreCode(whichServer, ServerData[whichServer].iCodeAreaAddr+index*sizeof(TInt)));
			brks->iCounter--;
		 	}
		 else
		 	{//Set breakpoint
		 	brk = 1;
			test_KErrNone(Device.WriteCode(whichServer, ServerData[whichServer].iCodeAreaAddr+index*sizeof(TInt),TestFuncCode.iNOP, /*size*/4));
			brks->iCounter++;	 	
		 	}
		}
	
	test.Printf(_L("Breakpoints left:%d\n"), brks->iCounter);

	//Check the content of the CodeArea in both XIP and Non-XIP Server
	for (i=0; i<2050;i++)
		{
		whichServer = i>1024 ? 1 : 0;
		if (i<=1024)index = i;
		else		index = i-1025;

		test_KErrNone(Device.ReadWord(whichServer,ServerData[whichServer].iCodeAreaAddr+index*sizeof(TInt), &var));
		if(brks->iBreakpoint[index][whichServer])
			test(var == TestFuncCode.iNOP); //Well, breakpoint is actually NOP ...
		else
			test(var == TestFuncCode.iBRK); //... while the original content is BRK instruction
		}

	//Now, apply breakpoints on all remaining addresses
	for (i=0; i<2050;i++)
		{
		whichServer = i>1024 ? 1 : 0;
		if (i<=1024)index = i;
		else		index = i-1025;

		if(!brks->iBreakpoint[index][whichServer])
			test_KErrNone(Device.WriteCode(whichServer, ServerData[whichServer].iCodeAreaAddr+index*sizeof(TInt),TestFuncCode.iNOP, /*size*/4));
		}

	//All BRKs are replaced with NOPs in CodeArea function in both Servers. It should be safe to call the function.
	test_KErrNone(RCodeModifierSession::RunCodeAreaFunc(EXip));
	test_KErrNone(RCodeModifierSession::GetServerInfo(EXip));//This will check the server is still alive
	test_KErrNone(RCodeModifierSession::RunCodeAreaFunc(ENonxip));
	test_KErrNone(RCodeModifierSession::GetServerInfo(ENonxip));//This will check the server is still alive

	test_KErrNone(Device.CloseCodeModifier()); //This will also remove all breakpoints
	delete brks;
	}

//Tests out-of-breakpoints scenario
void Test11(TInt aServer)
	{
	test.Printf(_L("Test11 %d\n"), aServer);
	test_KErrNone(Device.InitialiseCodeModifier(10));

	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr+4, /*aValue*/0, /*size*/4));

	//4 bytes breakpoint
	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr, /*aValue*/0, /*size*/4));
	test_Equal(KErrAccessDenied, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr, /*aValue*/0, /*size*/2));
	test_Equal(KErrAccessDenied, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr+2, /*aValue*/0, /*size*/2));
	test_Equal(KErrAccessDenied, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr, /*aValue*/0, /*size*/1));
	test_Equal(KErrAccessDenied, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr+1, /*aValue*/0, /*size*/1));
	test_Equal(KErrAccessDenied, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr+2, /*aValue*/0, /*size*/1));
	test_Equal(KErrAccessDenied, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr+3, /*aValue*/0, /*size*/1));
	test_KErrNone(Device.RestoreCode(aServer, ServerData[aServer].iCodeAreaAddr));

	//2 bytes breakpoint aligned to word
	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr, /*aValue*/0, /*size*/2));
	test_Equal(KErrAccessDenied, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr, /*aValue*/0, /*size*/4));
	test_Equal(KErrAccessDenied, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr, /*aValue*/0, /*size*/1));
	test_Equal(KErrAccessDenied, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr+1, /*aValue*/0, /*size*/1));
	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr+2, /*aValue*/0, /*size*/2));
	test_KErrNone(Device.RestoreCode(aServer, ServerData[aServer].iCodeAreaAddr));
	test_KErrNone(Device.RestoreCode(aServer, ServerData[aServer].iCodeAreaAddr+2));

	//2 bytes breakpoint aligned to word+2
	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr+2, /*aValue*/0, /*size*/2));
	test_Equal(KErrAccessDenied, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr, /*aValue*/0, /*size*/4));
	test_Equal(KErrAccessDenied, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr+2, /*aValue*/0, /*size*/1));
	test_Equal(KErrAccessDenied, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr+3, /*aValue*/0, /*size*/1));
	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr, /*aValue*/0, /*size*/2));
	test_KErrNone(Device.RestoreCode(aServer, ServerData[aServer].iCodeAreaAddr));
	test_KErrNone(Device.RestoreCode(aServer, ServerData[aServer].iCodeAreaAddr+2));

	//1 byte breakpoint
	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr, /*aValue*/0, /*size*/1));
	test_Equal(KErrAccessDenied, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr, /*aValue*/0, /*size*/4));
	test_Equal(KErrAccessDenied, Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr, /*aValue*/0, /*size*/2));
	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr+1, /*aValue*/0, /*size*/1));
	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr+2, /*aValue*/0, /*size*/1));
	test_KErrNone(Device.WriteCode(aServer, ServerData[aServer].iCodeAreaAddr+3, /*aValue*/0, /*size*/1));
	test_KErrNone(Device.RestoreCode(aServer, ServerData[aServer].iCodeAreaAddr));
	test_KErrNone(Device.RestoreCode(aServer, ServerData[aServer].iCodeAreaAddr+1));
	test_KErrNone(Device.RestoreCode(aServer, ServerData[aServer].iCodeAreaAddr+2));
	test_KErrNone(Device.RestoreCode(aServer, ServerData[aServer].iCodeAreaAddr+3));

	test_KErrNone(Device.CloseCodeModifier());
	}


void ClientAppL()
	{
	test.Start(_L("ClientAppL"));
	UpdateClientsGlobals();
	StartDriver();	
	StartAndUpdate(EXip);
	StartAndUpdate(ENonxip);
	
// All tests run with the following pre-conditions:
// - both XIP and nonXIP server are running.
// - device driver has updated servers' threadIDs (used for setting breakpoints and reading/writing data).
// - all global variables (ServerData, TestFuncCode)are valid.
// - CodeModifier is not installed.
	Test1();
	Test2(EXip);
	Test2(ENonxip);
	Test3(EXip);
	Test3(ENonxip);
	Test4(EXip);
	Test4(ENonxip);
	Test5(EXip);
	Test6(EXip);
	Test7(EXip);
	Test8(EXip);
	Test9();
	Test10();
	Test11(EXip);
	Test11(ENonxip);
	TerminateServer(EXip);
	TerminateServer(ENonxip);
	StopDriver();	
	test.End();
	}

/**Entry point for both client and server apps*/
TInt E32Main()
	{
	//Chech if we are client, XIP server or nonXIP server
	TBuf<64> c;
	User::CommandLine(c);
	if (c.FindF(KServerXIP) >= 0) 
		return ServerEntryPoint(EXip);
	if (c.FindF(KServerNONXIP) >= 0) 
		return ServerEntryPoint(ENonxip);
	
	// client
	CTrapCleanup* trap = CTrapCleanup::New();
	if (!trap)
		return KErrNoMemory;
	test.Title();
	__UHEAP_MARK;
	TRAPD(r,ClientAppL());
	__UHEAP_MARKEND;
	delete trap;
	return r;
	}
