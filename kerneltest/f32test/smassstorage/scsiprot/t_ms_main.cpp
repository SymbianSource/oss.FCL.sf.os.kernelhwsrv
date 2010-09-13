// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <f32file.h>
#include <e32test.h>
#include <e32math.h>
#include <f32dbg.h>
#include "t_ms_main.h"


GLDEF_D RTest test(_L("T_MS_SCSI"));

//
// Do tests relative to session path
//
GLDEF_C void CallTestsL()

	{
	t_scsi_prot();
	}

//
// Do testing on aDrive
//
LOCAL_C void DoTests()
	{
	TRAPD(r,CallTestsL());
	if (r!=KErrNone)
		{
		test.Printf(_L("Error: Leave %d\n"),r);
		test(EFalse);
		}
	}

_LIT( KValueTestFailMsg, "ERROR Got %d expected %d" );
GLDEF_C void TestIfEqual( TInt aValue, TInt aExpected, TInt aLine, const char aFileName[])
	{
	if( aExpected != aValue )
		{
		TText filenameU[512];
		TUint i = 0;
		for (; (i < sizeof(filenameU)) && (aFileName[i] != (char)0); i++)
			{
			filenameU[i]=aFileName[i];
			}
		filenameU[i]=0;
		test.Printf( KValueTestFailMsg, aValue, aExpected );
		test.operator()( EFalse, aLine, &filenameU[0]);
		}
	}


GLDEF_C TInt E32Main()
    {
	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();
 	__UHEAP_MARK;

	test.Title();
	test.Start(_L("Starting tests..."));

	TTime timerC;
	timerC.HomeTime();
	
	DoTests();

	TTime endTimeC;
	endTimeC.HomeTime();
	TTimeIntervalSeconds timeTakenC;
	TInt r=endTimeC.SecondsFrom(timerC,timeTakenC);
	test(r==KErrNone);
	test.Printf(_L("Time taken for test = %d seconds\n"),timeTakenC.Int());
	test.End();
	test.Close();
	__UHEAP_MARKEND;
	delete cleanup;
	return(KErrNone);
    }

