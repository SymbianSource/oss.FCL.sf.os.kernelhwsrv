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
// e32test\device\d_lddtunraroundtimetest.inl
// 
//

#ifndef __KERNEL_MODE__
inline TInt RLddTest1::Open()
	{ return DoCreate(KLddName,TVersion(0,1,1),KNullUnit,NULL,NULL); }
inline TInt RLddTest1::Test_getTimerTicks(TUint &time)
	{ return DoControl(EGET_TIMERTICKS, (TAny *)&time); }
inline TInt RLddTest1::Test_getTimerCount(TUint &time)
    { return DoControl(EGET_TIMERTICKCOUNT, (TAny *)&time); }
inline TInt RLddTest1::Unload()
	{ return User::FreeLogicalDevice(KLddName); }
#endif
