// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Contains dummy exports required to maintain BC between
// the SD and SD+CPRM versions of the SDIO controller.
// 
//

/**
 @file dummyexp.h
 @internalTechnology
*/

#ifndef __DUMMYEXP_H__
#define __DUMMYEXP_H__

#include <drivers/sdcard.h>

class DDummySession : public DSDSession
	{
public:
	inline DDummySession(const TMMCCallBack& aCallBack) : DSDSession(aCallBack) {};
private:
	IMPORT_C void DummyExport1();
	IMPORT_C void DummyExport2();
	IMPORT_C void DummyExport3();
	IMPORT_C void DummyExport4();
	IMPORT_C void DummyExport5();
	IMPORT_C void DummyExport6();
	IMPORT_C void DummyExport7();
	IMPORT_C void DummyExport8();
	IMPORT_C void DummyExport9();
	IMPORT_C void DummyExport10();
	IMPORT_C void DummyExport11();
	IMPORT_C void DummyExport12();
	IMPORT_C void DummyExport13();
	IMPORT_C void DummyExport14();
	IMPORT_C void DummyExport15();

#ifdef __EABI__
	IMPORT_C void DummyExport16();
	IMPORT_C void DummyExport17();
	IMPORT_C void DummyExport18();
	IMPORT_C void DummyExport19();
#endif

private:
	TUint32 iDummy1;
	TUint32 iDummy2;
	};

typedef DSDStack DStackBase;
typedef DDummySession DSessionBase;

const TUint32 KMinCustomSession = KSDMinCustomSession;

#endif	// #ifndef __DUMMYEXP_H__
