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

LOCAL_D RTest test(_L("T_CP950.exe"));

_LIT16(Uni_1, "\x0053\x3125\x3122\xAAAA\x9673\x2593\xFA0C\x3000");
_LIT8(CP950_1, "\x53\xA3\xB6\xA3\xB3\x5F\xB3\xAF\xF9\xFE\xC9\x4A\xA1\x40");
_LIT16(Uni_2, "\x0032\xFFFD\xFE3E\xFFFD\xFFFD");
_LIT8(CP950_2, "\x32\x80\xA1\x70\xC1\x7F\xA7");

_LIT(KName,"CP950");
const TUid KPluginUid={0x10206A8C};


// Used for supressing warning in OOM tests
#define __UNUSED_VAR(var) var = var

//
/**
@SYMTestCaseID          SYSLIB-FATCHARSETCONV-CT-1776
@SYMTestCaseDesc	    Tests API behaviours of UnicodeConv class
@SYMTestPriority 	    High
@SYMTestActions  	    Tests for conversions from/to Unicode, using a function pointer
@SYMTestExpectedResults Test must not fail
*/
void Test()
	{
	test.Next(_L("@SYMTestCaseID:SYSLIB-FATCHARSETCONV-CT-1776"));
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
	TBuf16<20> unicode2;

	const TDesC16& unicode1(Uni_1);
	(*aConvertFromUnicodeL)(foreign1, unicode1); 	//testing conversion from Unicode
	TInt error = foreign1.Compare(CP950_1);

	test(error==0);
	foreign1.Zero();

	const TDesC8& foreign2(CP950_2);
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
	result = (*aIsLegalShortNameCharacter)(0x3000); //testing for a double byte character
	test(result==1);

	lib.Close();
	}

// test code for INC080460: FATCharsetConv panics - stops china build booting
void TestINC080460()
	{
	RLibrary lib;
	const TUidType serverUid(KNullUid,KNullUid,KPluginUid);

	// load the dll
	TInt returnValue = lib.Load(KName,serverUid);
	test(returnValue==0);

	// get a pointer to the specified ordinal function and call it
	TLibraryFunction function1 = lib.Lookup(1);
	TLibraryFunction function2 = lib.Lookup(2);

	//cast the function pointer f to a function of type void with two arguments
	typedef void (*TConvertFromUnicodeL)(TDes8&, const TDesC16&);
	TConvertFromUnicodeL ConvertFromUnicodeL = reinterpret_cast <TConvertFromUnicodeL> (function1);

	typedef void (*TConvertToUnicodeL)(TDes16&, const TDesC8&);
	TConvertToUnicodeL ConvertToUnicodeL = reinterpret_cast <TConvertToUnicodeL> (function2);

	// the problem in this defect is when the foreign buffer is not big enough the code panics
	// the solution is for the code to leave

	// the foreign buffer is 11 (8+3) and there are 17 unicode characters
	TBuf8<11> foreign;
	_LIT16(longUnicode, "\x0053\x3125\x3122\xAAAA\x0053\x3125\x3122\xAAAA\x0053\x3125\x3122\xAAAA\x0053\x3125\x3122\xAAAA");
	const TDesC16& testUnicode1(longUnicode);
	TInt error = KErrNone;

	// ConvertFromUnicodeL - check that this call leaves
	foreign.Zero();
	TRAP( error, ConvertFromUnicodeL(foreign, testUnicode1)); 	//testing conversion from Unicode
	test(error == KErrOverflow );

	// ConvertFromUnicodeL - test 1 character to long leaves
	_LIT16(longUnicode2, "FilenameEx\x3122");
	const TDesC16& testUnicode2(longUnicode2);
	foreign.Zero();
	error = KErrNone;
	TRAP( error, ConvertFromUnicodeL(foreign, testUnicode2)); 	//testing conversion from Unicode
	test(error == KErrOverflow );

	// ConvertFromUnicodeL - test 11 character does not leave
	_LIT16(longUnicode3, "FilenameExt");
	const TDesC16& testUnicode3(longUnicode3);
	foreign.Zero();
	error = KErrNone;
	TRAP( error, ConvertFromUnicodeL(foreign, testUnicode3)); 	//testing conversion from Unicode
	test(error == KErrNone );

	// check ConvertToUnicodeL leaves when the buffer is too small
	TBuf16<6> unicodeBuffer;
	_LIT8(sampletext, "this is far to big to fit");
	const TDesC8& sample(sampletext);

	unicodeBuffer.Zero();
	error = KErrNone;
	TRAP( error,ConvertToUnicodeL(unicodeBuffer,sample)); 	//testing conversion to Unicode
	test(error==KErrOverflow);

	// test 6 characters does NOT leave
	_LIT8( chars6, "abcdef");
	const TDesC8& sample6(chars6);
	unicodeBuffer.Zero();
	error = KErrNone;
	TRAP( error,ConvertToUnicodeL(unicodeBuffer,sample6)); 	//testing conversion to Unicode
	test(error==KErrNone);

	// test 7 characters does leave
	_LIT8( chars7, "abcdefg");
	const TDesC8& sample7(chars7);
	unicodeBuffer.Zero();
	error = KErrNone;
	TRAP( error,ConvertToUnicodeL(unicodeBuffer,sample7)); 	//testing conversion to Unicode
	test(error==KErrOverflow);

	// test when 7 foreign characters fits in 5 unicode it does NOT leave
	_LIT8( CP950_sample, "\x32\x80\xA1\x70\xC1\x7F\xA7");
	const TDesC8& sample7to5(CP950_sample);
	unicodeBuffer.Zero();
	error = KErrNone;
	TRAP( error,ConvertToUnicodeL(unicodeBuffer,sample7to5)); 	//testing conversion to Unicode
	test(error==KErrNone);

	lib.Close();
	}


/**
@SYMTestCaseID          SYSLIB-FATCHARSETCONV-CT-1847-0003
@SYMTestCaseDesc	    Tests API behaviours of UnicodeConv class as part of INC090073
@SYMTestPriority 	    High
@SYMTestActions  	    Tests for correct character conversion on certain chinese characters for CP936
@SYMTestExpectedResults Test must not fail
*/
void TestINC090073()
	{
	test.Next(_L(" @SYMTestCaseID:SYSLIB-FATCHARSETCONV-CT-1847-0003 "));
 	_LIT16(unicode, "\x7C91\x8026\x8160\x7633\x6DFC\x715C\x6600\x9785\x86D8\x7A37\x61A9\x80B1\x86A3\x89E5\x80F2\x9B48\x6C19\x7B71\x946B\x6B46\x6615");
	_LIT8(CP950Code, "\xD3\x4A\xBD\xA2\xDF\x73\xEA\x6F\xD9\xE7\xB7\xD4\xA9\xFB\xBB\xDF\xDB\xB1\xBD\x5E\xBE\xCD\xAA\xD0\xB0\x47\xDF\xFD\xD3\x6A\xEF\x69\xCB\x49\xDF\x4E\xF8\xCA\xDD\xF5\xA9\xFD");

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

	TBuf8<50> foreign1;

	foreign1.Zero();
	const TDesC16& unicode1(unicode);
	TRAPD(err,(*aConvertFromUnicodeL)(foreign1, unicode1)); 	//testing conversion from Unicode
	test(err==0);
	TInt error = foreign1.Compare(CP950Code);
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
	Test();
	TestINC090073();
	TestINC080460();
	OOMTest();
	}

GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;

	test.Title();
	test.Start(_L("CP950 test..."));

	CTrapCleanup* trapCleanup=CTrapCleanup::New();
	TRAPD(error, DoE32MainL());
	delete trapCleanup;

	test.End();
	test.Close();

	__UHEAP_MARKEND;
	return error;
	}
