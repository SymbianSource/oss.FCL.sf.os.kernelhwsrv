/*
* Copyright (c) 2003 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/

/**
 Contains dummy exports required to maintain BC between
 the SD and SD+CPRM versions of the SDIO controller.
 */

#ifndef __USE_CPRM__

#include <drivers/sdio/dummyexp.h>

LOCAL_C void UnsupportedExport(TInt aParam) { Kern::Fault("SDIO Invalid Export", aParam); }

EXPORT_C void DDummySession::DummyExport1()  { UnsupportedExport(1); }
EXPORT_C void DDummySession::DummyExport2()  { UnsupportedExport(2); }
EXPORT_C void DDummySession::DummyExport3()  { UnsupportedExport(3); }
EXPORT_C void DDummySession::DummyExport4()  { UnsupportedExport(4); }
EXPORT_C void DDummySession::DummyExport5()  { UnsupportedExport(5); }
EXPORT_C void DDummySession::DummyExport6()  { UnsupportedExport(6); }
EXPORT_C void DDummySession::DummyExport7()  { UnsupportedExport(7); }
EXPORT_C void DDummySession::DummyExport8()  { UnsupportedExport(8); }
EXPORT_C void DDummySession::DummyExport9()  { UnsupportedExport(9); }
EXPORT_C void DDummySession::DummyExport10() { UnsupportedExport(10); }
EXPORT_C void DDummySession::DummyExport11() { UnsupportedExport(11); }
EXPORT_C void DDummySession::DummyExport12() { UnsupportedExport(12); }
EXPORT_C void DDummySession::DummyExport13() { UnsupportedExport(13); }
EXPORT_C void DDummySession::DummyExport14() { UnsupportedExport(14); }
EXPORT_C void DDummySession::DummyExport15() { UnsupportedExport(15); }

#ifdef __EABI__
EXPORT_C void DDummySession::DummyExport16() { UnsupportedExport(16); }
EXPORT_C void DDummySession::DummyExport17() { UnsupportedExport(17); }
EXPORT_C void DDummySession::DummyExport18() { UnsupportedExport(18); }
EXPORT_C void DDummySession::DummyExport19() { UnsupportedExport(19); }
#endif

#endif	// #ifndef __USE_CPRM__
