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
// e32test\system\testprocess.h
// 
//

#ifndef __TESTPROCESS_H__
#define __TESTPROCESS_H__

class RTestProcess : public RProcess
	{
public:
	void Create(TUint32 aCapability,TTestProcessFunctions aFunction,TInt aArg1=-1,TInt aArg2=-1);
	void Create(TTestProcessFunctions aFunction,TInt aArg1=-1,TInt aArg2=-1);
	void Run();
	static TUint iInstanceCount;
	};

TUint RTestProcess::iInstanceCount = 0;

void RTestProcess::Create(TUint32 aCapability,TTestProcessFunctions aFunction,TInt aArg1,TInt aArg2)
	{
	TFileName source=RProcess().FileName();

	TFileName destination;
	_LIT(KTempPath,"c:\\system\\bin\\");
	_LIT(KTempPathSysBin,"c:\\sys\\bin\\");
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		destination = KTempPathSysBin;
	else
		destination = KTempPath;
	destination.Append(source.Mid(source.LocateReverse('\\')+1));
	destination.SetLength(source.Locate('.'));
	destination.Append((TText)'-');
	destination.AppendNum(++iInstanceCount,EHex);
	_LIT(KDotExe,".exe");
	destination.Append(KDotExe);

	TInt r;
	TBuf<128> b;
	b.Zero();
#ifdef __WINS__
	if(source[0]=='z' || source[0]=='Z')
		{
		b.Append(source.Mid(source.LocateReverse('\\')+1));
		}
	else
#endif
	b.Append(source);
	b.Append(' ');
	b.AppendNum((TUint)aCapability,EHex);
	b.Append(' ');
	b.Append(destination);
	RProcess p;
	_LIT(KSetcapExe,"setcap.exe");
	r = p.Create(KSetcapExe,b);
	test(r==KErrNone);
	TRequestStatus s;
	p.Logon(s);
	p.Resume();
	User::WaitForRequest(s);
	test(s==KErrNone);
	test(p.ExitType()==EExitKill);

	if(aArg1==-1)
		aArg1 = RProcess().Id();
	TBuf<512> commandLine;
	commandLine.AppendNum((TInt)aFunction);
	commandLine.Append(_L(" "));
	commandLine.AppendNum(aArg1);
	commandLine.Append(_L(" "));
	commandLine.AppendNum(aArg2);
	r = RProcess::Create(destination,commandLine);
	test(r==KErrNone);
	SetJustInTime(EFalse);
	}

void RTestProcess::Create(TTestProcessFunctions aFunction,TInt aArg1,TInt aArg2)
	{
	if(aArg1==-1)
		aArg1 = RProcess().Id();
	TBuf<512> commandLine;
	commandLine.Num((TInt)aFunction);
	commandLine.Append(_L(" "));
	commandLine.AppendNum(aArg1);
	commandLine.Append(_L(" "));
	commandLine.AppendNum(aArg2);
	TInt r = RProcess::Create(RProcess().FileName(),commandLine);
	test(r==KErrNone);
	SetJustInTime(EFalse);
	}

void RTestProcess::Run()
	{
	TRequestStatus s;
	Logon(s);
	Resume();
	User::WaitForRequest(s);
	test(s==KErrNone);
	test(ExitType()==EExitKill);
	CLOSE_AND_WAIT(*this);
	}

#endif
