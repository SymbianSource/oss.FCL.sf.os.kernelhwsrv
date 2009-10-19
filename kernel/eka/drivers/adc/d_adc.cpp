// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\adc\d_adc.cpp
// Generic ADC driver
// 
//


#include <adc.h>

/******************************************************
 * ADC Channel
 ******************************************************/
EXPORT_C TAdcChannel::TAdcChannel(TInt anAdc)
	:	iChannelId(-1), iCommandCount(0), iCommandList(NULL),
		iReadings(NULL)
	{
	iNext=NULL;
//	iPriority=0;
	if (anAdc>=0 && anAdc<DAdc::NumberOfAdcs)
		iAdc=DAdc::TheAdcs[anAdc];
	else
		iAdc=NULL;
	}

EXPORT_C void TAdcChannel::Read(TInt* aReadingBuffer)
//
// Initiate a reading of this channel
//
	{
	TInt irq=NKern::DisableAllInterrupts();
	if (!iNext)
		{
		iReadings=aReadingBuffer;
		iAdc->Add(this);
		}
	NKern::RestoreInterrupts(irq);
	}

EXPORT_C void TAdcChannel::Preamble()
//
// Default preamble does nothing
//
	{
	}

EXPORT_C void TAdcChannel::Postamble()
//
// Default postamble does nothing
//
	{
	}


/******************************************************
 * ADC Controller
 ******************************************************/
LOCAL_C void timerExpired(TAny* aPtr)
	{
	((DAdc*)aPtr)->TimerExpired();
	}

DAdc::DAdc()
	:	iTimer(timerExpired,this)
	{
//	iCurrentChannel=NULL;
//	iCurrentCommand=0;
//	iCommandPtr=0;
//	iCommandCount=0;
//	iMinPriority=0;
	}

DAdc::~DAdc()
	{
	}

EXPORT_C TInt DAdc::SetMinPriority(TInt anAdc, TInt aPriority)
	{
	if (anAdc<0 || anAdc>=NumberOfAdcs)
		return KErrArgument;
	if (aPriority<0 || aPriority>KNumAdcChannelPriorities)
		return KErrArgument;
	return TheAdcs[anAdc]->DoSetMinPriority(aPriority);
	}

TInt DAdc::DoSetMinPriority(TInt aPriority)
	{
	TInt irq=NKern::DisableAllInterrupts();
	if (aPriority<iMinPriority)
		{
		if (iList.iPresent[0])
			{
			TAdcChannel* pN=iList.First();
			if (pN->iPriority>=aPriority)
				{
				iList.Remove(pN);
				Execute(pN);
				}
			}
		}
	iMinPriority=aPriority;
	NKern::RestoreInterrupts(irq);
	return KErrNone;
	}

void DAdc::Add(TAdcChannel* aChannel)
//
// Queue another ADC reading request
//
	{
	if (iCurrentChannel || (aChannel->iPriority<iMinPriority))
		iList.Add(aChannel);
	else
		Execute(aChannel);
	}

void DAdc::Execute(TAdcChannel* aChannel)
//
// Begin execution of an ADC request
// This runs in an ISR or with interrupts disabled
//
	{
	aChannel->iNext=(TPriListLink*)1;	// so channel will not be queued again
	iCurrentChannel=aChannel;
	iCommandPtr=aChannel->iCommandList;
	iCommandCount=aChannel->iCommandCount;
	NextCommand();
	}

void DAdc::NextCommand()
//
// Perform the next command in the command list
// This runs in an ISR or with interrupts disabled
//
	{
	TBool wait=EFalse;
	TAdcChannel* pC=iCurrentChannel;
	if (iCommandCount)
		{
		TInt c=*iCommandPtr++;
		iCurrentCommand=c;
		iCommandCount--;
		if (c & EAdcCmdPreamble)
			pC->Preamble();
		if (c & EAdcCmdPostamble)
			pC->Postamble();
		if (c & EAdcCmdWait)
			{
			iTimer.OneShot(c & 0xffff);
			wait=ETrue;
			}
		else if (c & EAdcCmdReading)
			{
			StartConversion(pC->iChannelId);
			wait=ETrue;
			}
		}
	if (iCommandCount==0 && !wait)
		{
		iCurrentChannel=NULL;
		pC->iNext=NULL;
		if (iList.iPresent[0])
			{
			TAdcChannel* pN=iList.First();
			if (pN->iPriority>=iMinPriority)
				{
				iList.Remove(pN);
				Execute(pN);
				}
			}
		pC->Complete();
		}
	}

void DAdc::Start()
//
// Called on completion of initialisation to start processing requests
// This runs in an ISR
//
	{
	iCurrentChannel=NULL;
	if (iList.iPresent[0])
		{
		TAdcChannel* pN=iList.First();
		iList.Remove(pN);
		Execute(pN);
		}
	}

void DAdc::ConversionComplete(TInt aValue)
//
// Called when a conversion has completed
// This runs in an ISR
//
	{
	if ((iCurrentCommand & EAdcCmdDiscard)==0)
		*(iCurrentChannel->iReadings)++=aValue;
	NextCommand();
	}

void DAdc::TimerExpired()
//
// Called in ISR when timer expires
//
	{
	NextCommand();
	}

