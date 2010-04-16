// Copyright (c) 2009-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkernsmp\nk_bal.h
// 
//

/**
 @file
 @internalComponent
*/

#ifndef __NK_BAL_H__
#define __NK_BAL_H__
#include <cpudefs.h>
#include <nkern.h>

#define __LOAD_BALANCE_INFO_DEFINED__

struct SLbInfo
	{
	TUint64HL			iRecentTime;
	TUint64HL			iRecentCpuTime;
	TUint64HL			iRecentActiveTime;
	TUint16				iLbRunTime;
	TUint16				iLbActTime;
	TUint16				iLbRunAct;
	TUint16				iLbRunAvg;
	TUint16				iLbActAvg;
	TUint16				iLbRunActAvg;
	TUint8				iLbNomPri;
	TUint8				iLbHot;
	TUint8				iLbWarm;
	TUint8				iLbAffinity;
	TUint8				iLbHeavy;
	TUint8				iLbSpare1;
	TUint8				iLbSpare2;
	TUint8				iLbSpare3;
	TUint8				iLbSpare4;
	TUint8				iLbSpare5;
	TUint8				iLbSpare6;
	TUint8				iLbSpare7;
	};

#define	DUMP_LOAD_BALANCE_INFO(s)	\
	Printf("RecentTime      %08x %08x\r\n", (s)->iLbInfo.iRecentTime.i32[1], (s)->iLbInfo.iRecentTime.i32[0]),	\
	Printf("RecentCpuTime   %08x %08x  RecentActivTime %08x %08x\r\n", (s)->iLbInfo.iRecentCpuTime.i32[1], (s)->iLbInfo.iRecentCpuTime.i32[0], (s)->iLbInfo.iRecentActiveTime.i32[1], (s)->iLbInfo.iRecentActiveTime.i32[0]),	\
	Printf("LbRunTime %03x  LbActTime %03x     LbRunAct %03x\r\n", (s)->iLbInfo.iLbRunTime, (s)->iLbInfo.iLbActTime, (s)->iLbInfo.iLbRunAct),	\
	Printf("LbRunAvg  %03x   LbActAvg %03x  LbRunActAvg %03x\r\n", (s)->iLbInfo.iLbRunAvg, (s)->iLbInfo.iLbActAvg, (s)->iLbInfo.iLbRunActAvg),	\
	Printf("LbNomPri  %02x       LbHot %02x        LbWarm %02x   LbAffinity %02x\r\n", (s)->iLbInfo.iLbNomPri, (s)->iLbInfo.iLbHot, (s)->iLbInfo.iLbWarm, (s)->iLbInfo.iLbAffinity),	\
	Printf("LbHeavy   %02x", (s)->iLbInfo.iLbHeavy)

#endif
