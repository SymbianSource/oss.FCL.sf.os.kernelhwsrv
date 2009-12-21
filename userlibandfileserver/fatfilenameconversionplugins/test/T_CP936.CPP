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

LOCAL_D RTest test(_L("T_CP936.exe"));

_LIT16(Uni_1, "\x0053\x0059\x004D\x3125\x312F\x3122\xFFFF\x9673\xFA29");
_LIT8(CP936_1, "\x53\x59\x4D\xA8\xE5\x5F\xA8\xE2\x5F\xEA\x90\xFE\x4F");
_LIT16(Uni_2, "\x0032\x20AC\xFFFD\xFFFD\x7FB1");
_LIT8(CP936_2, "\x32\x80\xA1\x70\xC1\x7F\xC1\x7E");

_LIT(KName,"CP936");
const TUid KPluginUid={0x10206A91};


// Used for supressing warning in OOM tests
#define __UNUSED_VAR(var) var = var

/**
@SYMTestCaseID          SYSLIB-FATCHARSETCONV-CT-1778
@SYMTestCaseDesc	    Tests API behaviours of UnicodeConv class
@SYMTestPriority 	    High
@SYMTestActions  	    Tests for conversions from/to Unicode, using a function pointer
@SYMTestExpectedResults Test must not fail 
*/
void Test()
	{ 
	test.Next(_L(" @SYMTestCaseID:SYSLIB-FATCHARSETCONV-CT-1778 "));
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
	TInt error = foreign1.Compare(CP936_1);
	test(error==0);
	foreign1.Zero();
	
	const TDesC8& foreign2(CP936_2);
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
	result = (*aIsLegalShortNameCharacter)(0x4E02); //testing for a double byte character
	test(result==1);

	lib.Close();
	}

	/**
@SYMTestCaseID          SYSLIB-FATCHARSETCONV-CT-1847-0001
@SYMTestCaseDesc	    Tests API behaviours of UnicodeConv class as part of INC090073
@SYMTestPriority 	    High
@SYMTestActions  	    Tests for correct character conversion on certain chinese characters for CP936 
@SYMTestExpectedResults Test must not fail 
*/	
void TestINC090073()
	{
	test.Next(_L(" @SYMTestCaseID:SYSLIB-FATCHARSETCONV-CT-1847-0001 ")); 
 	_LIT16(unicode, "\x7CCD\x74EF\x8026\x8F8F\x94F3\x7633\x6DFC\x9785\x7F81\x7A37\x61A9\x80B1\x86A3\x89E5\x80F2\x9B48\x9E47\x6C19\x7B71\x946B\x6B46\x6615");
	_LIT8(CP932Code, "\xF4\xD9\xEA\xB1\xF1\xEE\xEA\xA3\xEF\xA5\xF1\xAC\xED\xB5\xF7\xB1\xEE\xBF\xF0\xA2\xED\xAC\xEB\xC5\xF2\xBC\xF6\xA1\xEB\xDC\xF7\xCC\xF0\xC2\xEB\xAF\xF3\xE3\xF6\xCE\xEC\xA7\xEA\xBF");

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
	test.Start(_L("CP936 test..."));

	CTrapCleanup* trapCleanup=CTrapCleanup::New();
	TRAPD(error, DoE32MainL());
	test(error==KErrNone);
	
	delete trapCleanup;
	
	test.End();
	test.Close();

	__UHEAP_MARKEND;
	return error;
	}
