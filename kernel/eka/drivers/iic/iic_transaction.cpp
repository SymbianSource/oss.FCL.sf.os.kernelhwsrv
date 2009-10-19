// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\iic_transaction.cpp
// IIC Transaction API Implementation
//

#include <drivers/iic_transaction.h>
#include <drivers/iic_channel.h>

EXPORT_C void TIicBusSlaveCallback::DfcFunc(TAny* aPtr)
    {
    TIicBusSlaveCallback* pCb = (TIicBusSlaveCallback*) aPtr;
	// pCb can be NULL if a client corrupt it after the dfc has been queued
    __ASSERT_DEBUG(pCb != NULL, Kern::Fault(KIicPanic,__LINE__));
	DIicBusChannelSlave* chan = pCb->iChannel;
	__ASSERT_DEBUG(chan != NULL, Kern::Fault(KIicPanic,__LINE__));

	chan->ProcessData(pCb->iTrigger, pCb); // Call PSL to fill in iReturn, iRxWords and/or iTxWords
	//
	// Only invoke the client's callback (and update the state machine) if asynchronous channel capture has
	// completed, or if one of the requested triggers has occurred or if a bus error has been witnessed
	TInt8 callbackTrig = 0;
	TInt callbackRet = 0;
	TInt nextSteps = chan->UpdateReqTrig(callbackTrig, callbackRet);
	if(nextSteps & DIicBusChannelSlave::EStopTimer)
		{
		chan->StopTimer();
		}
	if(nextSteps & DIicBusChannelSlave::EInvokeCb)
		{
		(pCb->iCallback)(pCb->iChannelId, callbackRet, (TInt)callbackTrig, pCb->iRxWords, pCb->iTxWords, pCb->iParam);
		// Callback now processed, so re-initialise callback object members
		pCb->iTrigger = 0;
		pCb->iReturn = KErrNone;
		pCb->iRxWords = 0;
		pCb->iTxWords = 0;
		}
	if(nextSteps & DIicBusChannelSlave::EStartTimer)
		{
		chan->StartTimerByState();
		}
    }
