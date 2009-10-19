// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\personality\example\init.cpp
// Test code for example RTOS personality.
// 
//

#include <kernel/kern_priv.h>
#include <personality/example/personality.h>
#include <personality/example/personality_int.h>
#include "ifcldd.h"

#define	OC_TASK			0

#define MSG_ID_INIT		1
#define MSG_ID_RUN		2
#define MSG_ID_RUN_P	3
#define	MSG_ID_RND_ISR	4
#define MSG_ID_DONE		5
#define	MSG_ID_DATA		6
#define	MSG_ID_FLUSH	7
#define MSG_ID_SEM_RPT	8
#define MSG_ID_RCV_RPT	9
#define MSG_ID_TM_RPT	10

typedef struct _random_isr_msg
	{
	msghdr			header;
	unsigned		random_isr_number;
	unsigned		extra;
	} random_isr_msg;

typedef struct _data_msg
	{
	msghdr			header;
	int				length;
	unsigned char	checksum;
	unsigned char	data[1];
	} data_msg;

typedef struct _report_msg
	{
	msghdr			header;
	int				pad;
	unsigned		count;
	unsigned		ok_count;
	unsigned		bad_count;
	} report_msg;

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

void RxMsg(TAny* aPtr);

TPMsgQ ThePMsgQ(&RxMsg, 0, Kern::DfcQue0(), 7);

NONSHARABLE_CLASS(DRtosIfcFactory) : public DLogicalDevice
	{
public:
	DRtosIfcFactory();
	virtual TInt Install();						//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel);	//overriding pure virtual
	};

class TRxQ
	{
public:
	TRxQ(DThread* aThread) : iFirst(0), iLast(0), iStatus(0), iPtr(0), iThread(aThread) {}
	TInt QueueReq(TRequestStatus* aStatus, TAny* aPtr);
	void AddMsg(msghdr* aM);
	inline TBool MsgPresent() {return iFirst!=NULL;}
	void CompleteReq();
	void CancelReq();
	void Close();
public:
	msghdr* iFirst;
	msghdr* iLast;
	TRequestStatus* iStatus;
	TAny* iPtr;
	DThread* iThread;
	};

TInt TRxQ::QueueReq(TRequestStatus* aStatus, TAny* aPtr)
	{
	if (iStatus)
		return KErrInUse;
	iStatus = aStatus;
	iPtr = aPtr;
	if (iFirst)
		CompleteReq();
	return KErrNone;
	}

void TRxQ::AddMsg(msghdr* aM)
	{
	aM->next = NULL;
	if (iLast)
		iLast->next = aM;
	else
		iFirst = aM;
	iLast = aM;
	if (iStatus)
		CompleteReq();
	}

void TRxQ::CompleteReq()
	{
	msghdr* m = iFirst;
	iFirst = m->next;
	if (!iFirst)
		iLast = NULL;
	TInt r = KErrNone;
	switch (m->msg_id)
		{
		case MSG_ID_DATA:
			{
			data_msg* dm = (data_msg*)m;
			r = Kern::ThreadRawWrite(iThread, iPtr, &dm->length, dm->length + 5, iThread);
			break;
			}
		case MSG_ID_SEM_RPT:
		case MSG_ID_RCV_RPT:
		case MSG_ID_TM_RPT:
			{
			report_msg* rpt = (report_msg*)m;
			rpt->pad = m->msg_id;
			r = Kern::ThreadRawWrite(iThread, iPtr, &rpt->pad, sizeof(SReport), iThread);
			break;
			}
		default:
			break;
		}
	free_mem_block(m);
	Kern::RequestComplete(iThread, iStatus, r);
	iStatus = NULL;
	}

void TRxQ::CancelReq()
	{
	if (iStatus)
		Kern::RequestComplete(iThread, iStatus, KErrCancel);
	iStatus = NULL;
	}

void TRxQ::Close()
	{
	CancelReq();
	while (iFirst)
		{
		msghdr* m = iFirst;
		iFirst = m->next;
		free_mem_block(m);
		}
	iFirst = NULL;
	iLast = NULL;
	}

NONSHARABLE_CLASS(DRtosIfc) : public DLogicalChannel
	{
public:
	DRtosIfc();
	virtual ~DRtosIfc();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual void HandleMsg(TMessageBase* aMsg);
public:
	TInt DoControl(TInt aFunc, TAny* a1, TAny* a2);
	TInt DoRequest(TInt aFunc, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	void DoCancel(TInt aMask);
	void HandleRtosMsg(msghdr* aMsg);
public:
	DThread* iThread;
	TRequestStatus* iDoneStatus;
	TRxQ iRxQ;
	TRxQ iRptQ;
	};

void RxMsg(TAny* aPtr)
	{
	msghdr* m = ThePMsgQ.Get();
	((DRtosIfc*)aPtr)->HandleRtosMsg(m);
	}

TInt EntryPoint()
	{
	TPMsgQ::ThePMsgQ = &::ThePMsgQ;
	init_personality();

	assert(current_task_id() == TASK_ID_UNKNOWN);

	kprintf("Entry point exit");

	return 0;
	}

void SendInitMsg()
	{
	kprintf("Send init msg");
	msghdr* m = (msghdr*)alloc_mem_block(sizeof(msghdr));
	m->msg_id = MSG_ID_INIT;
	int r = send_msg(OC_TASK, m);
	assert(r == OK);
	}

void SendFlushMsg()
	{
	msghdr* m = (msghdr*)alloc_mem_block(sizeof(msghdr));
	m->msg_id = MSG_ID_FLUSH;
	int r = send_msg(OC_TASK, m);
	assert(r == OK);
	}

void SendFinishMsg()
	{
	msghdr* m = (msghdr*)alloc_mem_block(sizeof(msghdr));
	m->msg_id = MSG_ID_DONE;
	int r = send_msg(OC_TASK, m);
	assert(r == OK);
	}

DRtosIfcFactory::DRtosIfcFactory()
	{
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    //iParseMask=0;//No units, no info, no PDD
    //iUnitsMask=0;//Only one thing
	}

TInt DRtosIfcFactory::Create(DLogicalChannelBase*& aChannel)
    {
	aChannel = new DRtosIfc;
    return aChannel ? KErrNone : KErrNoMemory;
    }

TInt DRtosIfcFactory::Install()
	{
	TInt r = SetName(&KRtosIfcLddName);
#ifndef __EPOC32__
	if (r == KErrNone)
		r = EntryPoint();
#endif
	return r;
	}

void DRtosIfcFactory::GetCaps(TDes8& aDes) const
    {
    TCapsRtosIfcV01 b;
    b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
    }

DRtosIfc::DRtosIfc()
	:	iRxQ(&Kern::CurrentThread()),
		iRptQ(&Kern::CurrentThread())
    {
	iThread=&Kern::CurrentThread();
	iThread->Open();
    }

DRtosIfc::~DRtosIfc()
	{
	iRxQ.Close();
	iRptQ.Close();
	Kern::SafeClose((DObject*&)iThread, NULL);
	}

TInt DRtosIfc::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create channel
//
    {

    if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
    	return KErrNotSupported;
	if (ThePMsgQ.iPtr)
		return KErrInUse;
	SetDfcQ(Kern::DfcQue0());
	iMsgQ.Receive();
	ThePMsgQ.iPtr = this;
	ThePMsgQ.Receive();
	return KErrNone;
	}

void DRtosIfc::HandleMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m=*(TThreadMessage*)aMsg;
	TInt id=m.iValue;
	if (id==(TInt)ECloseMsg)
		{
		ThePMsgQ.CancelReceive();
		ThePMsgQ.iPtr = NULL;
		m.Complete(KErrNone,EFalse);
		iMsgQ.CompleteAll(KErrServerTerminated);
		return;
		}
	else if (id==KMaxTInt)
		{
		// DoCancel
		DoCancel(m.Int0());
		m.Complete(KErrNone,ETrue);
		return;
		}

	if (id<0)
		{
		// DoRequest
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		TInt r=DoRequest(~id,pS,m.Ptr1(),m.Ptr2());
		if (r!=KErrNone)
			Kern::RequestComplete(iThread,pS,r);
		m.Complete(KErrNone,ETrue);
		}
	else
		{
		// DoControl
		TInt r=DoControl(id,m.Ptr0(),m.Ptr1());
		m.Complete(r,ETrue);
		}
	}

TInt DRtosIfc::DoControl(TInt aFunc, TAny* a1, TAny* a2)
	{
	(void)a1;
	(void)a2;
	TInt r = KErrNone;
	switch (aFunc)
		{
		case RRtosIfc::EControlInit:
			SendInitMsg();
			break;
		case RRtosIfc::EControlFlush:
			SendFlushMsg();
			break;
		case RRtosIfc::EControlFinish:
			SendFinishMsg();
			break;
		case RRtosIfc::EControlSend:
			{
			data_msg* dm = (data_msg*)alloc_mem_block(512);
			TPtr8 lptr(dm->data, 0, 516-sizeof(data_msg));
			r = Kern::ThreadDesRead(iThread, a1, lptr, 0, 0);
			if (r == KErrNone)
				{
				dm->header.msg_id = MSG_ID_DATA;
				dm->length = lptr.Length();
				dm->checksum = 0;
				send_msg(OC_TASK, &dm->header);
				}
			else
				free_mem_block(dm);
			break;
			}
		default:
			r = KErrNotSupported;
			break;
		}
	return r;
	}

TInt DRtosIfc::DoRequest(TInt aFunc, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	(void)a1;
	(void)a2;
	switch (aFunc)
		{
		case RRtosIfc::ERequestWaitInitialTests:
			iDoneStatus = aStatus;
			return KErrNone;
		case RRtosIfc::ERequestReceive:
			return iRxQ.QueueReq(aStatus, a1);
		case RRtosIfc::ERequestReport:
			return iRptQ.QueueReq(aStatus, a1);
		default:
			return KErrNotSupported;
		}
	}

void DRtosIfc::DoCancel(TInt aMask)
	{
	if (aMask & RRtosIfc::ECancelWaitInitialTests)
		{
		Kern::RequestComplete(iThread, iDoneStatus, KErrCancel), iDoneStatus=NULL;
		}
	if (aMask & RRtosIfc::ECancelReceive)
		iRxQ.CancelReq();
	if (aMask & RRtosIfc::ECancelReport)
		iRptQ.CancelReq();
	}

void DRtosIfc::HandleRtosMsg(msghdr* aM)
	{
	switch (aM->msg_id)
		{
		case MSG_ID_DONE:
			if (iDoneStatus)
				Kern::RequestComplete(iThread, iDoneStatus, KErrNone), iDoneStatus=NULL;
			break;
		case MSG_ID_DATA:
			iRxQ.AddMsg(aM);
			aM = NULL;
			break;
		case MSG_ID_SEM_RPT:
		case MSG_ID_RCV_RPT:
		case MSG_ID_TM_RPT:
			iRptQ.AddMsg(aM);
			aM = NULL;
			break;
		default:
			break;
		}
	if (aM)
		free_mem_block(aM);
	ThePMsgQ.Receive();
	}

#ifdef __EPOC32__
DECLARE_STANDARD_EXTENSION()
	{
	return EntryPoint();
	}

DECLARE_EXTENSION_LDD()
	{
	return new DRtosIfcFactory;
	}
#else
DECLARE_STANDARD_LDD()
	{
	return new DRtosIfcFactory;
	}
#endif

