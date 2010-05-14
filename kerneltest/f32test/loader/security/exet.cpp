// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\loader\exet.cpp
// 
//

#define __INCLUDE_DEPENDENCY_GRAPH

#include <e32svr.h>
#include "dllt.h"
#include "exetifc.h"
#include "dlltree.h"
#include <d_ldrtst.h>

#ifdef __VC32__
#pragma warning(disable:4706)
#endif

const TInt KMaxHandlesPerDll=4;
const TInt KMaxHandles=KMaxHandlesPerDll*KNumModules;

extern "C" TInt _E32Startup();

extern "C" __MODULE_IMPORT void RegisterConstructorCall(TInt aDllNum);
extern "C" __MODULE_IMPORT void RegisterInitCall(TInt aDllNum);
extern "C" __MODULE_IMPORT void RegisterDestructorCall(TInt aDllNum);

#define PANIC()		ExeTPanic(__LINE__)
#define EXET_ASSERT(c)	((void)((c)||(PANIC(),0)))

void ExeTPanic(TInt aLine)
	{
	User::Panic(_L("EXET"),aLine);
	}

/******************************************************************************
 * Class Definitions
 ******************************************************************************/
class CDllInfo;
class TDllList : public MDllList
	{
public:
	TDllList();
	virtual TBool IsPresent(const SDllInfo& aInfo);
	virtual TInt Add(const SDllInfo& aInfo);
	virtual void MoveToEnd(TInt aPos);
public:
	TInt iCount;
	SDllInfo iInfo[KNumModules];
	};

class CTestServer : public CServer2
	{
public:
	CTestServer();
	virtual ~CTestServer();
	virtual CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;
	virtual TInt RunError(TInt aError);
public:
	CDllInfo* iInfo;
	};

class CTestSession : public CSession2
	{
public:
	virtual ~CTestSession();
	virtual void CreateL();
	virtual void ServiceL(const RMessage2& aMessage);
public:
	TInt GetExeDepList(const RMessage2& aMessage);
	TInt GetCDList(const RMessage2& aMessage);
	TInt LoadDll(const RMessage2& aMessage);
	TInt CloseDll(TInt aHandle);
	TInt CallBlkI(TInt aHandle, TInt aIn);
	TInt CallRBlkI(TInt aHandle, TInt aIn);
public:
	CDllInfo* iInfo;
	};

class CDllInfo : public CBase
	{
public:
	CDllInfo();
	virtual ~CDllInfo();
	TInt Create();
	TInt StoreHandle(TInt aHandle);
	void RemoveHandle(TInt aIndex);
	void SetupCDList();
public:
	void RegisterConstructorCall(TInt aDllNum);
	void RegisterInitCall(TInt aDllNum);
	void RegisterDestructorCall(TInt aDllNum);
public:
	TInt iNextGen;
	TInt iHandleCount;
	TInt iNextHandle;
	TInt iHandles[KMaxHandles];
	TInt iModuleNum[KMaxHandles];
	TDllList iExeDepList;
	TDllList iDllConstructList;
	};

/******************************************************************************
 * Static data
 ******************************************************************************/
#ifdef __MODULE_HAS_DATA
class TExeData
	{
public:
	TExeData();
public:
	TTime iStartTime;
	TTime iInitTime;
	TInt iTest1;
	TFileName iFileName;
	};

TInt Bss[16];
TInt ExeNum=EXENUM;
TInt Generation=0;
TInt InitFlag=0;
TFullName StartThread=RThread().FullName();
TName StartProcess=RProcess().Name();
TExeData TheExeDataObject;

TExeData::TExeData()
	:	iFileName(RProcess().FileName())
	{
	TInt r;
	CHKDEPS(r);		// Check our dependencies are initialised
	if (r!=KErrNone)
		User::Panic(_L("CHKDEPS"),r);
	iStartTime.HomeTime();
	iTest1=299792458;
	RegisterConstructorCall(EXENUM);
	}
#endif

/******************************************************************************
 * Class TDllList
 ******************************************************************************/
TDllList::TDllList()
	{
	iCount=0;
	Mem::Fill(iInfo, KNumModules*sizeof(SDllInfo), 0xff);
	}

TBool TDllList::IsPresent(const SDllInfo& aInfo)
	{
	TInt i;
	for (i=0; i<iCount; ++i)
		{
		if (iInfo[i].iDllNum==aInfo.iDllNum)
			return ETrue;
		}
	return EFalse;
	}

TInt TDllList::Add(const SDllInfo& aInfo)
	{
	EXET_ASSERT(iCount<KNumModules);
	TInt pos=iCount;
	iInfo[iCount++]=aInfo;
	return pos;
	}

void TDllList::MoveToEnd(TInt aPos)
	{
	if (aPos<iCount-1)
		{
		SDllInfo x(iInfo[aPos]);
		Mem::Move(iInfo+aPos, iInfo+aPos+1, (iCount-aPos-1)*sizeof(SDllInfo));
		iInfo[iCount-1]=x;
		}
	}

/******************************************************************************
 * Class CTestSession/CTestServer
 ******************************************************************************/
void ExceptionHandler(TExcType)
	{
	User::Leave(KErrGeneral);
	}

TUint32 Read(TLinAddr a)
	{
	return *(volatile TUint32*)a;
	}

TInt CheckReadable(TLinAddr a)
	{
	User::SetExceptionHandler(ExceptionHandler,KExceptionFault);
	TRAPD(r,Read(a));
	User::SetExceptionHandler(ExceptionHandler,0);
	return r;
	}

CTestSession::~CTestSession()
	{
	}

void CTestSession::CreateL()
	{
//	CSession2::CreateL();
	}

void CTestSession::ServiceL(const RMessage2& aMessage)
	{
	TInt r=KErrNotSupported;
	TInt mid=aMessage.Function();
	switch(mid)
		{
		case RLoaderTest::EMsgGetExeDepList:
			r=GetExeDepList(aMessage);
			break;
		case RLoaderTest::EMsgLoadDll:
			r=LoadDll(aMessage);
			break;
		case RLoaderTest::EMsgCallBlkI:
			r=CallBlkI(aMessage.Int0(), aMessage.Int1());
			break;
		case RLoaderTest::EMsgCallRBlkI:
			r=CallRBlkI(aMessage.Int0(), aMessage.Int1());
			break;
		case RLoaderTest::EMsgCloseDll:
			r=CloseDll(aMessage.Int0());
			break;
		case RLoaderTest::EMsgGetCDList:
			r=GetCDList(aMessage);
			break;
		case RLoaderTest::EMsgCheckReadable:
			r=CheckReadable((TLinAddr)aMessage.Int0());
			break;
		case RLoaderTest::EMsgExit:
			r=KErrNone;
			CActiveScheduler::Stop();
			break;
		default:
			break;
		}
	aMessage.Complete(r);
	}

TInt CTestSession::GetExeDepList(const RMessage2& aMsg)
	{
	TPtrC8 dep_list_ptr((const TUint8*)&iInfo->iExeDepList.iInfo, KNumModules*sizeof(SDllInfo));
	aMsg.WriteL(0, dep_list_ptr, 0);
	return KErrNone;
	}

TInt CTestSession::GetCDList(const RMessage2& aMsg)
	{
	TPtrC8 list_ptr((const TUint8*)&iInfo->iDllConstructList.iInfo, KNumModules*sizeof(SDllInfo));
	aMsg.WriteL(0, list_ptr, 0);
	return KErrNone;
	}

TInt CTestSession::LoadDll(const RMessage2& aMsg)
	{
	TInt module=aMsg.Int0();
	TDllList dll_list;
	TPtrC8 dll_list_ptr((const TUint8*)dll_list.iInfo, KNumModules*sizeof(SDllInfo));
	TPtrC dllname=MODULE_FILENAME(module);
	iInfo->SetupCDList();
	RLibrary l;
	TInt r=l.Load(dllname, TUidType());
	if (r!=KErrNone)
		return r;
	TInitFunction f=(TInitFunction)l.Lookup(INIT_ORDINAL);
	EXET_ASSERT(f);
	r=(*f)(dll_list);
	if (r!=KErrNone)
		return r;
	TBlkIFunction bf=(TBlkIFunction)l.Lookup(BLOCK_INC_ORDINAL);
	EXET_ASSERT(bf);
	TInt result=(*bf)(531441);
	EXET_ASSERT(result==BlkIValue(module, 531441));
	TInt h=iInfo->StoreHandle(l.Handle());
	EXET_ASSERT(h>=0);
	iInfo->iModuleNum[h]=module;
	aMsg.WriteL(1, dll_list_ptr, 0);
	return h;
	}

TInt CTestSession::CallBlkI(TInt aHandle, TInt aIn)
	{
	TInt h=iInfo->iHandles[aHandle];
	EXET_ASSERT(h!=0);
	RLibrary l;
	l.SetHandle(h);
	TBlkIFunction bf=(TBlkIFunction)l.Lookup(BLOCK_INC_ORDINAL);
	EXET_ASSERT(bf);
	return (*bf)(aIn);
	}

TInt CTestSession::CallRBlkI(TInt aHandle, TInt aIn)
	{
	TInt h=iInfo->iHandles[aHandle];
	EXET_ASSERT(h!=0);
	RLibrary l;
	l.SetHandle(h);
	TRBlkIFunction rbf=(TRBlkIFunction)l.Lookup(REC_BLOCK_INC_ORDINAL);
	EXET_ASSERT(rbf);
	++iInfo->iNextGen;
	return (*rbf)(aIn,iInfo->iNextGen);
	}

TInt CTestSession::CloseDll(TInt aHandle)
	{
	TInt h=iInfo->iHandles[aHandle];
	TInt m=iInfo->iModuleNum[aHandle];
	EXET_ASSERT(h!=0);
	EXET_ASSERT(m>=0);
	iInfo->iHandles[aHandle]=0;
	iInfo->iModuleNum[aHandle]=-1;
	iInfo->SetupCDList();
	RLibrary l;
	l.SetHandle(h);
	l.Close();
	return KErrNone;
	}

CTestServer::CTestServer()
	: CServer2(0,ESharableSessions)
	{
	}

CTestServer::~CTestServer()
	{
	}

CSession2* CTestServer::NewSessionL(const TVersion& aVersion, const RMessage2&) const
	{
	(void)aVersion;
	CTestSession* s = new (ELeave) CTestSession;
	s->iInfo=iInfo;
	return s;
	}

_LIT(KExetErr,"EXETERR");
TInt CTestServer::RunError(TInt aError)
	{
	User::Panic(KExetErr,aError);
	return 0;
	}

/******************************************************************************
 * Class CDllInfo
 ******************************************************************************/
TInt ChkC()
	{
#ifdef __MODULE_HAS_DATA
	TInt init_mark=~((EXENUM+DLLNUMOFFSET)*(EXENUM+DLLNUMOFFSET));
	if (InitFlag==init_mark)
		return KErrNone;
	if (InitFlag!=0)
		return 0x494e4946;
	TInt i;
	TInt x=0;
	for (i=0; i<16; ++i) x|=Bss[i];
	if (x)
		return 0x425353;
	if (ExeNum!=EXENUM)
		return 0x44415441;
	if (Generation!=0)
		return 0x47454e;
	if (StartProcess!=RProcess().Name())
		return 0x535450;
	if (TheExeDataObject.iTest1!=299792458)
		return 0x54455354;
	if (TheExeDataObject.iFileName != RProcess().FileName())
		return 0x464e414d;
	InitFlag=init_mark;
	RDebug::Print(_L("ChkC %S OK"),&TheExeDataObject.iFileName);
#endif
	return KErrNone;
	}

TInt Init(MDllList& aList)
	{
	TInt r=KErrNone;
	SDllInfo info;
	info.iDllNum=DLLNUM;
	info.iEntryPointAddress=((TInt)&_E32Startup);
	RLdrTest ldd;
	ldd.Open();
	info.iModuleHandle=ldd.ModuleHandleFromAddr((TInt)&_E32Startup);
	ldd.Close();
	if (!aList.IsPresent(info))
		{
		TInt pos=aList.Add(info);
		INITDEPS(r,aList);		// Call Init on our dependencies
		aList.MoveToEnd(pos);
#ifdef __MODULE_HAS_DATA
		if (r==KErrNone)
			r=ChkC();		// Check initial values for .data/.bss and check constructors have been called
#endif
		RegisterInitCall(DLLNUM);
		}
	return r;
	}

CDllInfo::CDllInfo()
	{
	Mem::Fill(iModuleNum, sizeof(iModuleNum), 0xff);
	}

CDllInfo::~CDllInfo()
	{
	}

TInt CDllInfo::Create()
	{
	TInt r;
	r=UserSvr::DllSetTls(0, this);
	if (r==KErrNone)
		r=UserSvr::DllSetTls(TLS_INDEX, NULL);
	if (r==KErrNone)
		{
		r=Init(iExeDepList);
		}
	return r;
	}

void CDllInfo::RegisterConstructorCall(TInt aDllNum)
	{
	(void)aDllNum;
	}

void CDllInfo::RegisterInitCall(TInt aDllNum)
	{
	(void)aDllNum;
	}

void CDllInfo::RegisterDestructorCall(TInt aDllNum)
	{
	(void)aDllNum;
	}

TInt CDllInfo::StoreHandle(TInt aHandle)
	{
	if (iHandleCount==KMaxHandles)
		return KErrOverflow;
	TInt i=iNextHandle;
	for (; i<KMaxHandles && iHandles[i]!=0; ++i) {}
	if (i==KMaxHandles)
		{
		for (i=0; i<iNextHandle && iHandles[i]!=0; ++i) {}
		EXET_ASSERT(i!=iNextHandle);
		}
	iNextHandle=i;
	iHandles[i]=aHandle;
	++iHandleCount;
	return i;
	}

void CDllInfo::RemoveHandle(TInt aIndex)
	{
	iHandles[aIndex]=0;
	--iHandleCount;
	}

void CDllInfo::SetupCDList()
	{
	new (&iDllConstructList) TDllList;
	EXET_ASSERT(UserSvr::DllSetTls(TLS_INDEX, &iDllConstructList)==KErrNone);
	}

/******************************************************************************
 * Exports
 ******************************************************************************/
extern "C" __MODULE_EXPORT void RegisterConstructorCall(TInt aDllNum)
	{
	CDllInfo* p=(CDllInfo*)UserSvr::DllTls(0);
	if (p)
		p->RegisterConstructorCall(aDllNum);
	}

extern "C" __MODULE_EXPORT void RegisterInitCall(TInt aDllNum)
	{
	CDllInfo* p=(CDllInfo*)UserSvr::DllTls(0);
	if (p)
		p->RegisterInitCall(aDllNum);
	}

extern "C" __MODULE_EXPORT void RegisterDestructorCall(TInt aDllNum)
	{
	CDllInfo* p=(CDllInfo*)UserSvr::DllTls(0);
	if (p)
		p->RegisterDestructorCall(aDllNum);
	}

GLDEF_C TInt E32Main()
	{
	CTrapCleanup* cleanup=CTrapCleanup::New();
	EXET_ASSERT(cleanup);
	CActiveScheduler* sched=new CActiveScheduler;
	EXET_ASSERT(sched);
	CActiveScheduler::Install(sched);
	CTestServer* svr=new CTestServer;
	EXET_ASSERT(svr);
	TBuf<16> suffix;
	User::CommandLine(suffix);
	TName svr_name=KServerName();
	if (suffix.Length())
		{
		svr_name.Append('.');
		svr_name+=suffix;
		}
	EXET_ASSERT(svr->Start(svr_name)==KErrNone);
	CDllInfo* dllinfo=new CDllInfo;
	EXET_ASSERT(dllinfo);
	EXET_ASSERT(dllinfo->Create()==KErrNone);
	svr->iInfo=dllinfo;

	CActiveScheduler::Start();

	UserSvr::DllFreeTls(0);
	UserSvr::DllFreeTls(TLS_INDEX);
	delete dllinfo;
	delete svr;
	delete sched;
	delete cleanup;

	return 0;
	}
