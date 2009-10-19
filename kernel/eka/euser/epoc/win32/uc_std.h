// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\epoc\win32\uc_std.h
// 
//

#if !defined(__UCSTD_H__)
#define __UCSTD_H__
#include <e32def.h>
#include <e32wins.h>

#ifndef _WINDOWS_H
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>
#endif

enum TCdtArchitecturePanic
    {
	ELazyDispatch,
	EBootstrap
    };

GLREF_C void Panic(TCdtArchitecturePanic aPanic);

#endif
