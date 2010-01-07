// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\smpsoak\d_smpsoak.h
//

#if !defined(__D_SMPSOAK_H__)
#define __D_RNDTIM_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KSmpSoakLddName,"SmpSoak");


class RSMPSoak : public RBusLogicalChannel
	{
public:

	enum TCommands
		{
		KGETPROCESSORCOUNT,
   	    KGETCURRENTCPU,
   	    KGETCURRENTTHREAD,
		KTHREADSETCPUAFFINITY,
		KOCCUPYCPUS,
		KCHANGEAFFINITY,
		KCHANGETHREADPRIORITY
		};
	
#ifndef __KERNEL_MODE__
	inline TInt Open()
		{ return DoCreate(KSmpSoakLddName(),TVersion(0,1,1),KNullUnit,NULL,NULL); }
	inline TInt TryControl(TInt aCommand, TInt aTestNum)
		{ return DoControl((TInt)aCommand,(TAny*)aTestNum); }
	inline TInt ChangeThreadAffinity(RThread* aThread, TInt cpu)
		{ return DoControl((TInt)KCHANGEAFFINITY,(TAny*)aThread->Handle(), (TAny*) cpu); }
	inline TInt GetThreadCPU(RThread* aThread)
		{ return DoControl((TInt)KGETCURRENTCPU,(TAny*)aThread->Handle(), (TAny*) NULL); }
#endif
	};

#endif


