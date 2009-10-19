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
//

/**
 @file
 @internalComponent
*/

#ifndef __ARM_MEM_H__
#define __ARM_MEM_H__


#include <memmodel.h>
#include <mmboot.h>
#include <arm.h>


class DArmPlatThread : public DMemModelThread
	{
public:
	~DArmPlatThread();
	virtual TInt SetupContext(SThreadCreateInfo& anInfo);
	virtual TInt Context(TDes8& aDes);
	virtual void DoExit2();
public:
	friend class Monitor;
	};


class DArmPlatProcess : public DMemModelProcess
	{
public:
	virtual TInt GetNewThread(DThread*& aThread, SThreadCreateInfo& aInfo);
private:
	friend class Monitor;
	};


#endif	// __ARM_MEM_H__
