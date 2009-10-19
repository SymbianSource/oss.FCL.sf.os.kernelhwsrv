// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\pbus\pccard\epoc\pccd_medchg.cpp
// 
//

#include <pccd_medchg.h>

DPlatPcCardMediaChange::DPlatPcCardMediaChange(TInt aMediaChangeNum)
//
// Constructor
//
	:	DPcCardMediaChange(aMediaChangeNum),
		iDelayCallBack(DelayCallBack,this)
	{
    }

TInt DPlatPcCardMediaChange::Create()
//
// Initialiser.
//
	{

	TInt r=DPcCardMediaChange::Create();
	if (r==KErrNone)
		{
		iMedChgIntId=ThePccdCntrlInterface->IntIdMediaChange(iMediaChangeNum);
		r=Interrupt::Bind(iMedChgIntId,Isr,this);
		}
	__KTRACE_OPT(KPBUS1,Kern::Printf("<PlatMedCh:Init(M:%d)-%d",iMediaChangeNum,r));
	return(r);
	}

void DPlatPcCardMediaChange::Isr(TAny *aPtr)
//
// Handle the media change (this function, never postponed is called on media
// change interrupt). 
//
	{
	__KTRACE_OPT(KFAIL,Kern::Printf("!^"));
	DPlatPcCardMediaChange* pM=(DPlatPcCardMediaChange*)aPtr;
	Interrupt::Disable(pM->iMedChgIntId);
	ThePccdCntrlInterface->ClearMediaChange(pM->iMediaChangeNum);
	pM->DoorOpenService();
	}

void DPlatPcCardMediaChange::DelayCallBack(TAny *aPtr)
//
// Timer callback after media change
//
	{
	DPlatPcCardMediaChange* pM=(DPlatPcCardMediaChange*)aPtr;
	pM->DoorClosedService();
	}

void DPlatPcCardMediaChange::ForceMediaChange()
	{
	Interrupt::Disable(iMedChgIntId);
	ThePccdCntrlInterface->ClearMediaChange(iMediaChangeNum);
	DoorOpenService();
	}

void DPlatPcCardMediaChange::DoDoorOpen()
//
// Called after media change has been recognised
//
	{

	__KTRACE_OPT(KPBUS2,Kern::Printf(">PlatMedCh(%d):DoDoorOpen",iMediaChangeNum));
    iDelayCallBack.OneShot(NKern::TimerTicks(20),ETrue);
	}

void DPlatPcCardMediaChange::DoDoorClosed()
//
// Handle the media door closing (called on tick).
//
	{

	ThePccdCntrlInterface->ClearMediaChange(iMediaChangeNum);
	Interrupt::Enable(iMedChgIntId);
	}

TMediaState DPlatPcCardMediaChange::MediaState()
//
// Return status of media changed signal.
//
	{

	return ThePccdCntrlInterface->MediaState(iMediaChangeNum);
	}

