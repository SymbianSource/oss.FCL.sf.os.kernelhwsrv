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
// e32test\defrag\d_pagemove.h
// 
//

#if !defined(__D_PAGEMOVE_H__)
#define __D_PAGEMOVE_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif


_LIT(KPageMoveLddName,"d_pagemove");

class TCapsPageMoveV01
	{
public:
	TVersion	iVersion;
	};

class RPageMove : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlTryMovingKHeap,
		EControlTryMovingKStack,
		EControlTryMovingUserPage,
		EControlTryMovingKCode,
		EControlTryMovingLocale,
		EControlPerfMovingKData,
		EControlGetPhysAddr,
		EControlTryMovingPhysAddr,
		EControlTryMovingPageTable,
		EControlTryMovingPageTableInfo,
		EControlNumberOfCpus,
		};
		
public:
	inline TInt Open();
	inline TInt TryMovingKHeap();
	inline TInt TryMovingKStack();
	inline TInt TryMovingUserPage(TAny* aAddr, TBool aEchoOff=EFalse);
	inline TInt TryMovingKCode();
	inline TInt TryMovingLocaleDll(TAny *aAddr);
	inline TInt TestKernelDataMovePerformance(void);
	inline TInt GetPhysAddr(TAny* aLinAddr, TAny* aPhysAddr);
	inline TInt TryMovingPhysAddr(TAny* aPhysAddr, TAny* aNewPhysAddr);
	inline TInt TryMovingPageTable(TAny* aAddr);
	inline TInt TryMovingPageTableInfo(TAny* aAddr);
	inline TUint NumberOfCpus();
	};


#ifndef __KERNEL_MODE__
inline TInt RPageMove::Open()
	{
	TInt r=User::LoadLogicalDevice(KPageMoveLddName);
	if(r==KErrNone || r==KErrAlreadyExists)
		r=DoCreate(KPageMoveLddName,TVersion(0,1,1),KNullUnit,NULL,NULL);
	return r;
	}

inline TInt RPageMove::TryMovingKHeap()
	{ return DoControl(EControlTryMovingKHeap); }

inline TInt RPageMove::TryMovingKStack()
	{ return DoControl(EControlTryMovingKStack); }

inline TInt RPageMove::TryMovingUserPage(TAny* aAddr, TBool aEchoOff)
	{ return DoControl(EControlTryMovingUserPage, aAddr, (TAny*)aEchoOff); }

inline TInt RPageMove::TryMovingKCode()
	{ return DoControl(EControlTryMovingKCode); }

inline TInt RPageMove::TryMovingLocaleDll(TAny* aAddr)
	{ return DoControl(EControlTryMovingLocale, aAddr, (TAny*)EFalse); }

inline TInt RPageMove::TestKernelDataMovePerformance(void)
	{ return DoControl(EControlPerfMovingKData); }

inline TInt RPageMove::GetPhysAddr(TAny* aLinAddr, TAny* aPhysAddr)
	{ return DoControl(EControlGetPhysAddr, aLinAddr, aPhysAddr); }

inline TInt RPageMove::TryMovingPhysAddr(TAny* aPhysAddr, TAny* aNewPhysAddr)
	{ return DoControl(EControlTryMovingPhysAddr, aPhysAddr, aNewPhysAddr); }

inline TInt RPageMove::TryMovingPageTable(TAny* aLinAddr)
	{ return DoControl(EControlTryMovingPageTable, aLinAddr); }

inline TInt RPageMove::TryMovingPageTableInfo(TAny* aLinAddr)
	{ return DoControl(EControlTryMovingPageTableInfo, aLinAddr); }

inline TUint RPageMove::NumberOfCpus()
	{ return DoControl(EControlNumberOfCpus);}
#endif

#endif
