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
// e32\include\memmodel\epoc\direct\arm\arm_mem.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
*/

#ifndef __ARM_MEM_H__
#define __ARM_MEM_H__
#include <arm.h>
#include <memmodel.h>
#include <kernel/cache.h>

const TInt KRamBlockSize=0x1000;

#define __USE_CP15_FAULT_INFO__

class DArmPlatThread : public DThread
	{
public:
	~DArmPlatThread();
	virtual TInt Context(TDes8& aDes);
	virtual TInt SetupContext(SThreadCreateInfo& anInfo);
	virtual void DoExit2();
public:
	friend class Monitor;
	};

class DArmPlatProcess : public DMemModelProcess
	{
public:
	DArmPlatProcess();
	~DArmPlatProcess();
public:
	virtual TInt GetNewThread(DThread*& aThread, SThreadCreateInfo& anInfo);
public:
	friend class Monitor;
	};

#endif	// __ARM_MEM_H__
