/*
* Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
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


#include <e32test.h>
#include <e32des8.h>

LOCAL_D RTest test(_L("T_CP1255.exe"));

_LIT16(Uni_1, "\x0053\x0059\x004D\x0042\x0049\x0041\x004E\x3125\x05D0\xFFFF");
_LIT8(CP1255_1, "\x53\x59\x4D\x42\x49\x41\x4E\x5F\xE0\x5F");
_LIT16(Uni_2, "\x0032\x00A1\x0070\x05B1\xFFFD\xFFFD\x20AC");
_LIT8(CP1255_2, "\x32\xA1\x70\xC1\x8E\x81\x80");

_LIT(KName,"CP1255");
const TUid KPluginUid={0x10206A97};


// Used for supressing warning in OOM tests
#define __UNUSED_VAR(var) var = var

/**
@SYMTestCaseID          SYSLIB-FATCHARSETCONV-CT-1784
@SYMTestCaseDesc	    Tests API behaviours of UnicodeConv class
@SYMTestPriority 	    High
@SYMTestActions  	    Tests for conversions from/to Unicode, using a function pointer
@SYMTestExpectedResults Test must not fail
*/
void Test()
	{
	RLibrary lib;

	const TUidType serverUid(KNullUid,KNullUid,KPluginUid);
	// load the dll
	TInt returnValue = lib.Load(KName,serverUid);
	test(returnValue==0);

	// get a pointer to the specified ordinal function and call it
	TLibraryFunction function1 = lib.Lookup(1);
	TLibraryFunction function2 = lib.Lookup(2);
	TLibraryFunction function3 = lib.Lookup(3);

	//cast the function pointer f to a function of type void with two arguments
	typedef void (*TConvertFromUnicodeL)(TDes8&, const TDesC16&);
	TConvertFromUnicodeL aConvertFromUnicodeL = reinterpret_cast <TConvertFromUnicodeL> (function1);

	typedef void (*TConvertToUnicodeL)(TDes16&, const TDesC8&);
	TConvertToUnicodeL aConvertToUnicodeL = reinterpret_cast <TConvertToUnicodeL> (function2);

	typedef TBool (*TIsLegalShortNameCharacter)(TUint);
	TIsLegalShortNameCharacter aIsLegalShortNameCharacter = reinterpret_cast <TIsLegalShortNameCharacter> (function3);


	TBuf8<15> foreign1;
	TBuf16<15> unicode2;

	const TDesC16& unicode1(Uni_1);
	(*aConvertFromUnicodeL)(foreign1, unicode1); 	//testing conversion from Unicode
	TInt error = foreign1.Compare(CP1255_1);
	test(error==0);
	foreign1.Zero();

	const TDesC8& foreign2(CP1255_2);
	(*aConvertToUnicodeL)(unicode2,foreign2); 	//testing conversion to Unicode
	error = unicode2.Compare(Uni_2);
	test(error==0);
	unicode2.Zero();


	//testing for legal short name character
	TInt result = (*aIsLegalShortNameCharacter)(0x005F); //testing for existent character
	test(result==1);
	result = (*aIsLegalShortNameCharacter)(0x003F); //testing for illegal character
	test(result==0);
	result = (*aIsLegalShortNameCharacter)(0x2999); //testing for non-existent character
	test(result==0);

	lib.Close();
	}

void OOMTest()
	{
	test.Next(_L("OOM testing"));
	TInt err, tryCount = 0;
	do
		{
			__UHEAP_MARK;
  		// find out the number of open handles
		TInt startProcessHandleCount;
		TInt startThreadHandleCount;
		RThread().HandleCount(startProcessHandleCount, startThreadHandleCount);

			// Setting Heap failure for OOM test
		__UHEAP_SETFAIL(RHeap::EDeterministic, ++tryCount);

		TRAP(err,Test());

		__UHEAP_SETFAIL(RHeap::ENone, 0);

		// check that no handles have leaked
		TInt endProcessHandleCount;
		TInt endThreadHandleCount;
		RThread().HandleCount(endProcessHandleCount, endThreadHandleCount);

		test(startProcessHandleCount == endProcessHandleCount);
		test(startThreadHandleCount  == endThreadHandleCount);

		__UHEAP_MARKEND;
		}while (err == KErrNoMemory);

	test(err == KErrNone);
	test.Printf(_L("- server succeeded at heap failure rate of %i\n"), tryCount);
	}


LOCAL_C void DoE32MainL()
	{
	Test();
	OOMTest();
	}



GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;

	test.Title();
	test.Start(_L(" @SYMTestCaseID:SYSLIB-FATCHARSETCONV-CT-1784 CP1255 test... "));

	CTrapCleanup* trapCleanup=CTrapCleanup::New();
	TRAPD(error, DoE32MainL());
	delete trapCleanup;

	test.End();
	test.Close();

	__UHEAP_MARKEND;
	return error;
	}
