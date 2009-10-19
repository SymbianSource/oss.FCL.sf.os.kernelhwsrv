// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef SDFIELDCHECK_H
#define SDFIELDCHECK_H

#include "sdbase.h"
#include "sdserver.h"

#define SYMBIAN_TEST_TESTNOPANIC(a) { if (!(a)) { ERR_PRINTF1(_L("Check failed")); SetTestStepResult(EFail); }}

#ifndef INFO_PRINTF9
#define INFO_PRINTF9(p1, p2, p3, p4, p5, p6, p7, p8, p9)        Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo, (p1), (p2), (p3), (p4), (p5), (p6), (p7), (p8), (p9))
#endif

/*
SD Test Step. Check the File System layout on the card conforms to the SD specification.
*/
class CBaseTestSDFieldCheck : public CBaseTestSDBase
	{
public:
	CBaseTestSDFieldCheck(CBaseTestSDServer& aOurServer);
	virtual TVerdict doTestStepPreambleL();
	virtual TVerdict doTestStepL();

private:
	/*
	IMPORTANT NOTE:
	FS1() and FS2() methods must be each invoked at least once prior to any other
	call. These two methods will initialise the private attributes of this
	class.
	*/
	void FS1();
	void FS2();
	void FS2Fat32();
	void FS3();
	void FS4();
	void FS5();
	void FSInfo();
	void FSBackupSectors();
	
	CBaseTestSDServer& iServer;
	};

_LIT(KTestStepFieldCheck, "FieldCheck");

#endif // SDFIELDCHECK_H
