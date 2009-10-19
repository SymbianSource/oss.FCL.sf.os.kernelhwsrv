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
// e32\include\memmodel\epoc\direct\x86\x86_mem.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
*/

#ifndef __X86_MEM_H__
#define __X86_MEM_H__
#include <x86.h>
#include <memmodel.h>

const TInt KRamBlockSize=0x1000;

typedef TInt (__fastcall *TMagicExcHandler)(TX86ExcInfo*);
class DX86PlatThread : public DThread
	{
public:
	DX86PlatThread();
	~DX86PlatThread();
	virtual TInt Context(TDes8& aDes);
	virtual TInt SetupContext(SThreadCreateInfo& anInfo);
	virtual void DoExit2();
public:
	friend class Monitor;
public:
	TMagicExcHandler iMagicExcHandler;
	};

class DX86PlatProcess : public DMemModelProcess
	{
public:
	DX86PlatProcess();
	~DX86PlatProcess();
public:
	virtual TInt GetNewThread(DThread*& aThread, SThreadCreateInfo& anInfo);
public:
	friend class Monitor;
	};

#endif	// __X86_MEM_H__
