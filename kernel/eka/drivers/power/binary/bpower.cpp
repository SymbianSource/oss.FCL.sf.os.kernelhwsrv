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
// e32\drivers\power\binary\bpower.cpp
// Binary power model
// 
//

#include <assp.h>
#include <kernel/kern_priv.h>
#include <kernel/kpower.h>
#include "mconf.h"
#include <e32keys.h>

NONSHARABLE_CLASS(DBinaryBatteryMonitor) : public DBatteryMonitor
	{
public: // from DBatteryMonitor
	void SystemTimeChanged(TInt anOldTime, TInt aNewTime);
	TSupplyStatus MachinePowerStatus();
public:
	DBinaryBatteryMonitor();
	void SupplyInfo(TSupplyInfoV1& si);
	};
DBinaryBatteryMonitor* BinaryBatteryMonitor;

NONSHARABLE_CLASS(DBinaryPowerHal) : public DPowerHal
	{
public: // from DPowerHal
	TInt PowerHalFunction(TInt aFunction, TAny* a1, TAny* a2);
public:
	DBinaryPowerHal();
private:
	void InitData();
	};
DBinaryPowerHal* BinaryPowerHal;

DBinaryBatteryMonitor::DBinaryBatteryMonitor()
	{
	Register();
	}

void DBinaryBatteryMonitor::SystemTimeChanged(TInt /* aOldTime */, TInt /* aNewTime */)
	{
	}

void DBinaryBatteryMonitor::SupplyInfo(TSupplyInfoV1& si)
	{
	si.iMainBatteryStatus = EZero;
	si.iMainBatteryMilliVolts = 0;
	si.iMainBatteryMaxMilliVolts = 0;
	si.iBackupBatteryStatus = EZero;
	si.iBackupBatteryMilliVolts = 0;
	si.iBackupBatteryMaxMilliVolts = 0;
	si.iMainBatteryInsertionTime = 0;
	si.iMainBatteryInUseMicroSeconds = 0;
	si.iMainBatteryConsumedMilliAmpSeconds = 0;
	si.iExternalPowerPresent = 0;
	si.iExternalPowerInUseMicroSeconds = 0;
	si.iCurrentConsumptionMilliAmps = 0;
	si.iFlags = 0;
	}

TSupplyStatus DBinaryBatteryMonitor::MachinePowerStatus()
	{
	return EGood;
	}


DBinaryPowerHal::DBinaryPowerHal()
	{
	Register();
	// initialise persistent data on cold start
	if (Kern::ColdStart())
		InitData();
	}

void DBinaryPowerHal::InitData()
	{
	TActualMachineConfig& mc = TheActualMachineConfig();
	TOnOffInfoV1& i = mc.iOnOffInfo;
	i.iPointerSwitchesOn = EFalse;
	i.iCaseOpenSwitchesOn = EFalse;
	i.iCaseCloseSwitchesOff = EFalse;
	}

TInt DBinaryPowerHal::PowerHalFunction(TInt aFunction, TAny* a1, TAny* /* a2 */)
	{
	__KTRACE_OPT(KPOWER,Kern::Printf("DBinaryPowerHal::PowerHalFunction() func=0x%x, a1=0x%x", aFunction, a1));
	TActualMachineConfig& mc=TheActualMachineConfig();
	TInt r=KErrNone;
	switch(aFunction)
		{
		case EPowerHalSwitchOff:
			if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EPowerHalSwitchOff")))
				return KErrPermissionDenied;
			{
			TRawEvent v;
			v.Set(TRawEvent::ESwitchOff);
			Kern::AddEvent(v);
			}
			break;
		case EPowerHalOnOffInfo:
			Kern::InfoCopy(*(TDes8*)a1, (TUint8*)&mc.iOnOffInfo, sizeof(mc.iOnOffInfo));
			break;
		case EPowerHalSupplyInfo:
			{
			TSupplyInfoV1 si;
			BinaryBatteryMonitor->SupplyInfo(si);
			Kern::InfoCopy(*(TDes8*)a1, (TUint8*)&si, sizeof(si));
			break;
			}
		case EPowerHalSetPointerSwitchesOn:
			if(!Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal Function EPowerHalSetPointerSwitchesOn")))
				return KErrPermissionDenied;
			mc.iOnOffInfo.iPointerSwitchesOn = (TUint8)(TUint)a1;
			break;	
		case EPowerHalPointerSwitchesOn:
			kumemput32(a1, &mc.iOnOffInfo.iPointerSwitchesOn, sizeof(TBool));
			break;
		case EPowerHalSetCaseOpenSwitchesOn:
			if(!Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EPowerHalSetCaseOpenSwitchesOn")))
				return KErrPermissionDenied;
			mc.iOnOffInfo.iCaseOpenSwitchesOn = (TUint8)(TUint)a1;
			break;
		case EPowerHalCaseOpenSwitchesOn:
			kumemput32(a1, &mc.iOnOffInfo.iCaseOpenSwitchesOn, sizeof(TBool));
			break;
		case EPowerHalSetCaseCloseSwitchesOff:
			if(!Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EPowerHalSetCaseCloseSwitchesOff")))
				return KErrPermissionDenied;
			mc.iOnOffInfo.iCaseCloseSwitchesOff = (TUint8)(TUint)a1;
			break;
		case EPowerHalCaseCloseSwitchesOff:
			kumemput32(a1, &mc.iOnOffInfo.iCaseCloseSwitchesOff, sizeof(TBool));
			break;
		case EPowerHalTestBootSequence:
			r = EFalse;
			break;
		case EPowerHalBackupPresent:
			{
			TBool ret=EFalse;
			kumemput32(a1, &ret, sizeof(TBool));
			}
			break;
		case EPowerHalAcessoryPowerPresent:
			{
			TBool ret=EFalse;
			kumemput32(a1, &ret, sizeof(TBool));
			}
			break;
		default:
			r = KErrNotSupported;
			break;
		}
	return r;
	}

TInt BinaryPowerInit()
	{
	__KTRACE_OPT(KPOWER,Kern::Printf("BinaryPowerInit()"));
	BinaryBatteryMonitor = new DBinaryBatteryMonitor();
	if (!BinaryBatteryMonitor)
		return KErrNoMemory;
	BinaryPowerHal = new DBinaryPowerHal();
	if (!BinaryPowerHal)
		return KErrNoMemory;
	return KErrNone;
	}
