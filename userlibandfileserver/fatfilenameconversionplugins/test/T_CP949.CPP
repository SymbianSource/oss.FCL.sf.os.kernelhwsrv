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

LOCAL_D RTest test(_L("T_CP949.exe"));

_LIT16(Uni_1, "\xFFFF\x0053\x0059\x004D\x0042\x0049\x0041\x004E\xAC21\xC6AA\xFFFF\x9673\x8A70");
_LIT8(CP949_1, "\x5F\x53\x59\x4D\x42\x49\x41\x4E\x81\x4D\x9F\x42\x5F\xF2\xE7\xFD\xFE");
_LIT16(Uni_2, "\xFFFD\x005F\x304A\xFFFD\xFFFD");
_LIT8(CP949_2, "\xAA\x40\x5F\xAA\xAA\xFF\xFE");

_LIT(KName,"CP949");
const TUid KPluginUid={0x10206A90};


// Used for supressing warning in OOM tests
#define __UNUSED_VAR(var) var = var

/**
@SYMTestCaseID          SYSLIB-FATCHARSETCONV-CT-1777
@SYMTestCaseDesc	    Tests API behaviours of UnicodeConv class
@SYMTestPriority 	    High
@SYMTestActions  	    Tests for conversions from/to Unicode, using a function pointer
@SYMTestExpectedResults Test must not fail
*/
void Test()
	{
	test.Next(_L(" @SYMTestCaseID:SYSLIB-FATCHARSETCONV-CT-1777 "));
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


	TBuf8<20> foreign1;
	TBuf16<15> unicode2;

	const TDesC16& unicode1(Uni_1);
	TRAPD(err,(*aConvertFromUnicodeL)(foreign1, unicode1)); 	//testing conversion from Unicode
	test(err==0);
	TInt error = foreign1.Compare(CP949_1);
	test(error==0);
	foreign1.Zero();

	const TDesC8& foreign2(CP949_2);
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
	result = (*aIsLegalShortNameCharacter)(0xAC02); //testing for double byte character
	test(result==1);

	lib.Close();
	}

/**
@SYMTestCaseID          SYSLIB-FATCHARSETCONV-CT-1847-0002
@SYMTestCaseDesc	    Tests API behaviours of UnicodeConv class as part of INC090073
@SYMTestPriority 	    High
@SYMTestActions  	    Tests for correct character conversion on certain chinese characters for CP936
@SYMTestExpectedResults Test must not fail
*/
void TestINC090073()
	{
	test.Next(_L(" @SYMTestCaseID:SYSLIB-FATCHARSETCONV-CT-1847-0002 "));
 	_LIT16(unicode, "\x715C\x7A37\x61A9\x80B1\x86A3\x6B46\x6615");
	_LIT8(CP932Code, "\xE9\xF2\xF2\xC3\xCC\xA8\xCE\xDD\xCD\xF7\xFD\xE4\xFD\xDA");

	RLibrary lib;

	const TUidType serverUid(KNullUid,KNullUid,KPluginUid);
	// load the dll
	TInt returnValue = lib.Load(KName,serverUid);
	test(returnValue==0);

	// get a pointer to the specified ordinal function and call it
	TLibraryFunction function1 = lib.Lookup(1);


	//cast the function pointer f to a function of type void with two arguments
	typedef void (*TConvertFromUnicodeL)(TDes8&, const TDesC16&);
	TConvertFromUnicodeL aConvertFromUnicodeL = reinterpret_cast <TConvertFromUnicodeL> (function1);

	TBuf8<15> foreign1;

	foreign1.Zero();
	const TDesC16& unicode1(unicode);
	TRAPD(err,(*aConvertFromUnicodeL)(foreign1, unicode1)); 	//testing conversion from Unicode
	test(err==0);
	TInt error = foreign1.Compare(CP932Code);
	test(error==0);
	foreign1.Zero();

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
	__UHEAP_MARK;

	Test();
	TestINC090073();
	OOMTest();

	__UHEAP_MARKEND;
	}

GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;

	test.Title();
	test.Start(_L("CP949 test..."));

	CTrapCleanup* trapCleanup=CTrapCleanup::New();
	TRAPD(error, DoE32MainL());
	test(error==KErrNone);

	delete trapCleanup;

	test.End();
	test.Close();

	__UHEAP_MARKEND;
	return error;
	}
