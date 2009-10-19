// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\arm\cipc.cpp
// 
//

#include "arm_mem.h"
#include "execs.h"

#ifdef __MESSAGE_MACHINE_CODED__

GLREF_C void PanicBadWriteAddress();
GLREF_C void PanicMesAlreadyPending();

#ifdef KIPC
extern "C" void __DebugMsgComplete(TInt a, TInt b)
	{
	// a->message, b=reason
	if (KDebugNum(KIPC))
		{
		RMessageK* m = (RMessageK*)a;
		Kern::Printf("MsgCo: M:%d r:%d %O->%O",m->iFunction,b,TheCurrentThread,m->iClient);
		}
	}

extern "C" void __DebugMsgAccept(TInt a, TInt b)
	{
	// a->DServer, b->RMessageK
	if (KDebugNum(KIPC))
		{
		DServer* svr = (DServer*)a;
		RMessageK* m = (RMessageK*)b;
		TInt f = m->iFunction;
		if (f==RMessage2::EDisConnect)
			{
			Kern::Printf("MsgAcD: %O->%O", m->iSession, svr);
			}
		else
			{
			Kern::Printf("MsgAc: M:%d %O->%O", f, m->iClient, svr);
			}
		}
	}

extern "C" void __DebugMsgAccept2(TInt a, TInt b)
	{
	if (KDebugNum(KIPC))
		{
		Kern::Printf("SvrSig: run %08x home %08x", a, a+b);
		}
	}

extern "C" void __DebugMsgSessionSend(TInt a, TInt b, TInt c)
	{
	if (KDebugNum(KIPC))
		{
		DSession* s = (DSession*)a;
		if (c)
			Kern::Printf("SessSS: M:%d %O(%08x)->%O", b, TheCurrentThread, s, s->iServer);
		else
			Kern::Printf("SessSA: M:%d %O(%08x)->%O", b, TheCurrentThread, s, s->iServer);
		}
	}

extern "C" void __DebugMsgServerReceive(TInt a, TInt b, TInt c)
	{
	if (KDebugNum(KIPC))
		{
		DServer* svr = (DServer*)a;
		Kern::Printf("SvrRx: %O msgd=%08x status=%08x iStatus=%08x %1d", svr, c, b, svr->iMessage->StatusPtr(),
			svr->iDeliveredQ.IsEmpty()?0:1);
		}
	}

extern "C" void __DebugMsgSendServerTerminated()
	{
	if (KDebugNum(KIPC))
		{
		Kern::Printf("DSession::Send: Server terminated");
		}
	}

extern "C" void __DebugMsgSendSessionClosed()
	{
	if (KDebugNum(KIPC))
		{
		Kern::Printf("DSession::Send: Session closed");
		}
	}

#endif //KIPC
#endif //__MESSAGE_MACHINE_CODED__
