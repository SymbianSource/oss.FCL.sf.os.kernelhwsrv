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
// e32test\mmu\t_wsd_tst.h
//

#include <e32std.h>
#include <e32debug.h>

#ifndef __T_WSD_TST_H__
#define __T_WSD_TST_H__

#if		defined(__MARM_ARM4__)
// ARM4: definition of IMPORT_D is wrong?
#undef	IMPORT_D
#define	IMPORT_D	 __declspec(dllimport)
#endif	// defined(__MARM_ARM4__)

#if		defined(__MARM_ARMV5__)
// ARMV5: definition of IMPORT_D is wrong?
#undef	IMPORT_D
#define	IMPORT_D	 __declspec(dllimport)
#endif	// defined(__MARM_ARMV5__)

#if		defined(__MSVC6__)
// MSVC6: definition of IMPORT_D is wrong?
#undef	IMPORT_D
#define	IMPORT_D	 __declspec(dllimport)
#endif	// defined(__MSVC6__)

#if		defined(__WINS__)
// WINSCW: definition of IMPORT_D is wrong?
#undef	IMPORT_D
#define	IMPORT_D	 __declspec(dllimport)
#endif	// defined(__WINS__)

// Exports of DLL1
#ifdef	T_WSD_DL1
#else
IMPORT_D extern TInt32 ExportedData;
IMPORT_C TInt CheckExportedDataAddress(void *aDataAddr);
#endif	// T_WSD_DL1

// Exports of DLL2
#ifdef	T_WSD_DL2
#else
typedef void** (*TGetAddressOfDataFunction)(TInt&, void*&, void*&);
const TInt KGetAddressOfDataFunctionOrdinal = 1;
IMPORT_C void** GetAddressOfDataProxy(TInt& aSize, void*& aCodeAddr, void*& aDataAddr);
const TInt KCheckWritableStaticDataFunctionOrdinal = 2;
IMPORT_C TInt CheckWritableStaticData(void);
#endif	// T_WSD_DL2

// Exports of DLL3
#ifdef	T_WSD_DL3
#else
IMPORT_C void** GetAddressOfData(TInt& aSize, void*& aCodeAddr, void*& aDataAddr);
IMPORT_D extern TInt32 TestDataSize;
IMPORT_D extern void* WritableTestData[1 /* refer TestDataSize */];
IMPORT_D extern const void* const* PointerToStaticData;
IMPORT_D extern void** PointerToWritableData;
#endif	// T_WSD_DL3

#endif	// __T_WSD_TST_H__
