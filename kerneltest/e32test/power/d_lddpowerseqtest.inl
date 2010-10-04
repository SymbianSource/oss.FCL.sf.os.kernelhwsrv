// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\power\d_lddpowerseqtest.inl
// 
//

#ifndef __KERNEL_MODE__
inline TInt RLddTest1::Open()
	{ return DoCreate(KLddName,TVersion(0,1,1),KNullUnit,NULL,NULL); }
inline void RLddTest1::Test_power1down(TRequestStatus &aStatus, TUint &time)
	{ DoRequest(EPOWERDOWN_POWER1, aStatus, (TAny *)&time); }
inline void RLddTest1::Test_power2down(TRequestStatus &aStatus, TUint &time)
	{ DoRequest(EPOWERDOWN_POWER2, aStatus, (TAny *)&time); }
inline void RLddTest1::Test_power1up(TRequestStatus &aStatus, TUint &time)
	{ DoRequest(EPOWERUP_POWER1, aStatus, (TAny *)&time); }
inline void RLddTest1::Test_power2up(TRequestStatus &aStatus, TUint &time)
	{ DoRequest(EPOWERUP_POWER2, aStatus, (TAny *)&time); }
inline TInt RLddTest1::Test_setSleepTime(TUint time)
	{ return DoControl(ESET_SLEEPTIME, (TAny *)time); }
inline void RLddTest1::Test_power2ActDead()
	{ DoControl(EPOWER_ACTDEAD_POWER2); }
inline void RLddTest1::Test_setPowerDownTimeout(TUint aTimeout)
	{ DoControl(EPOWER_ESETPOWERDOWNTIMEOUT, (TAny *) aTimeout); }
inline TInt RLddTest1::Unload()
	{ return User::FreeLogicalDevice(KLddName); }
#endif
