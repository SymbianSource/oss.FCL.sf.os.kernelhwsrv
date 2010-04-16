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
// e32test\mmu\t_wsd_dl2.cpp
// 

// MMH file will define	T_WSD_DL2
#include "t_wsd_tst.h"

// Ordinal 1
EXPORT_C void** GetAddressOfDataProxy(TInt& aSize, void*& aCodeAddr, void*& aDataAddr)
	{
	return GetAddressOfData(aSize, aCodeAddr, aDataAddr);
	}

// Ordinal 2
EXPORT_C TInt CheckWritableStaticData(void)
	{
	RDebug::Printf("CheckWritableStaticData: start");
	TInt err = KErrNone;
	TInt size;
	void* codeAddr;
	void* dataAddr;
	const void*** dataPtrPtr = (const void***)GetAddressOfData(size, codeAddr, dataAddr);

	RDebug::Printf("CheckWritableStaticData: size %d, codeAddr %08x, dataAddr %08x",
		size, codeAddr, dataAddr);
	RDebug::Printf("CheckWritableStaticData: dataPtrPtr %08x, PointerToStaticData is at %08x",
		dataPtrPtr, &PointerToStaticData);

	if (dataPtrPtr != (const void***)&PointerToStaticData)
		err = KErrGeneral;
	const void** p1 = *dataPtrPtr;
	RDebug::Printf("CheckWritableStaticData: *dataPtrPtr %08x", p1);
	const void* const* p2 = PointerToStaticData;
	RDebug::Printf("CheckWritableStaticData: PointerToStaticData %08x", p2);
	if (p1 != (const void**)p2)
		err = KErrGeneral;
	if (p1 != (const void**)dataAddr)
		err = KErrGeneral;

	RDebug::Printf("CheckWritableStaticData: TestDataSize is at %08x", &TestDataSize);
	TInt sz = TestDataSize;
	RDebug::Printf("CheckWritableStaticData: TestDataSize is %d", sz);
	if (sz != size)
		err = KErrGeneral;

	void** p3 = WritableTestData;
	void** p4 = PointerToWritableData;
	if (p3 != p4)
		err = KErrGeneral;

	return err;
	}

