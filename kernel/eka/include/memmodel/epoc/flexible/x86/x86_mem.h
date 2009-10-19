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
// e32\include\memmodel\epoc\multiple\x86\x86_mem.h
// 
//

/**
 @file
 @internalComponent
*/

#ifndef __X86_MEM_H__
#define __X86_MEM_H__


#include <memmodel.h>
#include <mmboot.h>
#include <x86.h>

typedef TInt (__fastcall *TMagicExcHandler)(TX86ExcInfo*);


class DX86PlatThread : public DMemModelThread
	{
public:
	DX86PlatThread();
	~DX86PlatThread();
	virtual TInt SetupContext(SThreadCreateInfo& aInfo);
	virtual TInt Context(TDes8& aDes);
	virtual void DoExit2();
public:
	TMagicExcHandler iMagicExcHandler;
public:
	friend class Monitor;
	};


class DX86PlatProcess : public DMemModelProcess
	{
public:
	virtual TInt GetNewThread(DThread*& aThread, SThreadCreateInfo& aInfo);
private:
	friend class Monitor;
	};


#endif	// __X86_MEM_H__
