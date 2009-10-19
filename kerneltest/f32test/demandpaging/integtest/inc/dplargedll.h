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
//

extern "C" IMPORT_C TUint32 Read1();
extern "C" IMPORT_C TUint32 Read2();
extern "C" IMPORT_C TUint32 Read3();
extern "C" IMPORT_C TUint32 Read4();
extern "C" IMPORT_C TUint32 Read5();
extern "C" IMPORT_C TUint32 Read6();
extern "C" IMPORT_C TUint32 Read7(TInt);
extern "C" EXPORT_C TUint32 Read8();
extern "C" EXPORT_C TUint32 Read9();
extern "C" EXPORT_C const TUint32* GetDataAddress();
extern "C" EXPORT_C TUint32 GetSizeOfData();

typedef enum
	{
	KTest1Page1Value = 0,
	KTest1Page2BoundariesValue,
	KTest2Page2Value,
	KTest2Page2BoundaryValue,
	KTest3Page4BoundaryValue,
	KTest128Page128Value,
	KTestNumberOfThread
	} ETestReadType;
