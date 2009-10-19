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
//

#ifndef __T_PROPERTY_H__
#define __T_PROPERTY_H__

#include "t_framework.h"

class CPropDefine : public CTestProgram
	{
public:
	CPropDefine(TUid aCategory, TUint aKey, RProperty::TType aType);
	void Run(TUint aCount);
private:

	TUid				iCategory;
	TUint				iKey;
	RProperty::TType	iType;
	};

class CPropDelete : public CTestProgram
	{
public:
	CPropDelete(TUid aCategory, TUint aKey, RProperty::TType aType);
	void Run(TUint aCount);
private:

	TUid				iCategory;
	TUint				iKey;
	RProperty::TType	iType;
	};

class CPropPanic : public CTestProgram
	{
public:
	CPropPanic(TUid aCategory, TUint aKey);
	void Run(TUint aCount);
private:
	static TInt DoubleSubscribeThreadEntry(TAny* ptr);
	static TInt BadHandleSubscribeThreadEntry(TAny* ptr);
	static TInt BadHandleCancelThreadEntry(TAny* ptr);
	static TInt BadHandleGetIThreadEntry(TAny* ptr);
	static TInt BadHandleGetBThreadEntry(TAny* ptr);
	static TInt BadHandleSetIThreadEntry(TAny* ptr);
	static TInt BadHandleSetBThreadEntry(TAny* ptr);

	static TThreadFunction BadHandles[];


	TUid				iCategory;
	TUint				iKey;
	};

class CPropSetGet : public CTestProgram
	{
public:
	CPropSetGet(TUid aCategory, TUint aKey, RProperty::TType aType); 
	void Run(TUint aCount);
private:

	TUid				iCategory;
	TUint				iKey;
	RProperty::TType	iType;
	};
		
class CPropSubsCancel : public CTestProgram
	{
public:
	CPropSubsCancel(TUid aCategory, TUint aKey, RProperty::TType aType);
	void Run(TUint aCount);
private:

	TUid				iCategory;
	TUint				iKey;
	RProperty::TType	iType;
	};

class CPropSecurity : public CTestProgram
	{
public:
	CPropSecurity(TUid aCategory, TUint aMasterKey, RProperty::TType aType, TUint aSlaveKeySlot);
	void Run(TUint aCount);

	struct TArgs
		{
		TUint				iCount;
		TUid				iCategory;
		TUint				iMasterKey;
		TUint				iSlaveKeySlot;
		};

private:
	TUid				iCategory;
	TUint				iMasterKey;
	TUint				iSlaveKeySlot;
	RProperty::TType	iType;
	};

class CPropSetGetRace : public CTestProgram
	{
public:
	CPropSetGetRace(TUid aCategory, TUint aKey);
	void Run(TUint aCount);
private:
	static TInt TrublemakerThreadEntry(TAny* ptr);
	void Trublemaker(TDes8& aBuf);

	TUid	iCategory;
	TUint	iKey;

	TBuf8<RProperty::KMaxPropertySize> iBuf1;
	TBuf8<RProperty::KMaxPropertySize> iBuf2;
	};

class CPropCancelRace : public CTestProgram
	{
public:
	CPropCancelRace(TUid aCategory, TUint aKey);
	void Run(TUint aCount);
private:
	static TInt TrublemakerThreadEntry(TAny* ptr);
	void Trublemaker();

	TUid	iCategory;
	TUint	iKey;

	RProperty	iProp;
	};

class CPropBroadcast : public CTestProgram
	{
public:
	CPropBroadcast(TUid aCategory, TUint aMasterKey, TUint aSlaveKeySlot, TUint aSlaveCount, TUint aFirstHighPriority);
	void Run(TUint aCount);

private:
	class CPropBroadcastSlave*	iSlaveList;

	TUid	iCategory;
	TUint	iMasterKey;
	TUint	iSlaveKeySlot;
	TUint	iSlaveCount;
	TUint	iFirstHighPriority;
	};

class CPropLddClient : public CTestProgram
	{
public:
	CPropLddClient(TUid aCategory, TUint aKey, RProperty::TType iType);
	void Run(TUint aCount);

private:

	TUid				iCategory;
	TUint				iKey;
	RProperty::TType	iType;

	};

static _LIT_SECURITY_POLICY_PASS(KPassPolicy);
static _LIT_SECURITY_POLICY_FAIL(KFailPolicy);

#endif
