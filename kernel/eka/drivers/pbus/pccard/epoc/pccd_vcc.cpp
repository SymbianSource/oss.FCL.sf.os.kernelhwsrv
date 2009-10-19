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
// e32\drivers\pbus\pccard\epoc\pccd_vcc.cpp
// 
//

#include <pccd_vcc.h>

DPlatPcCardVcc::DPlatPcCardVcc(TInt aPsuNum, TInt aMediaChangeNum)
//
// Constructor.
//
	:	DPcCardVcc(aPsuNum,aMediaChangeNum)
	{}

DPlatPcCardVcc::~DPlatPcCardVcc()
//
// Destructor
//
	{}

void DPlatPcCardVcc::PsuInfo(TPBusPsuInfo &anInfo)
//
// Return machine info relating to the Pc Card Vcc supply
//
	{

	ThePccdCntrlInterface->VccInfo(iPsuNum,anInfo);
    }

void DPlatPcCardVcc::DoSetState(TPBusPsuState aState)
//
// Turn on/off the PSU.
//
	{

    switch (aState)
        {
    case EPsuOff:
        ThePccdCntrlInterface->VccOff(iPsuNum);
		__KTRACE_OPT(KPBUS2,Kern::Printf("PlatPsu-Off"));
        break;
    case EPsuOnFull:
        ThePccdCntrlInterface->VccOnFull(iPsuNum,iVoltageSetting);
		__KTRACE_OPT(KPBUS2,Kern::Printf("PlatPsu-On"));
        break;
    case EPsuOnCurLimit:
        ThePccdCntrlInterface->VccOnCurrentLimit(iPsuNum,iVoltageSetting);
		__KTRACE_OPT(KPBUS2,Kern::Printf("PlatPsu-OnL"));
        break;
        }
	}

void DPlatPcCardVcc::DoCheckVoltage()
//
// Check the voltage level of the PSU is as expected. Returns either KErrNone, KErrGeneral 
// to indicate the pass/fail state or KErrNotReady if the voltage check isn't complete.
//
	{

	TInt err=KErrGeneral;
	if (iVoltCheckMethod==EPsuChkComparator)
		err=ThePccdCntrlInterface->VccVoltCheck(iPsuNum);
	ReceiveVoltageCheckResult(err);
	}

