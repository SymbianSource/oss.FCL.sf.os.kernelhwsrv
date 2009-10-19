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
// e32test\notifier\textnotifier.h
// 
//

const TUid KUidTestTextNotifier1 = {0x101fe1b9};
const TUid KUidTestTextNotifier2 = {0x101fe1ba};

_LIT8(KStartData,"Start");
_LIT8(KResponseData,"Response");
_LIT8(KUpdateData,"Update");
_LIT8(KStartWithCancelCheckData,"StartWithCancelCheck");
_LIT8(KBadData,"Bad");
_LIT8(KMNotifierManager,"MNotifierManager");
_LIT8(KMNotifierManagerWithCancelCheck,"MNotifierManagerWithCancelCheck");
_LIT8(KHeapData,"Heap");

const TInt KTestNotifierWasPreviouselyCanceled = 0x12345678;
