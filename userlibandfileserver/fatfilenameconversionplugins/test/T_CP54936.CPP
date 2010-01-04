/*
* Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include <hal.h>

LOCAL_D RTest test(_L("T_CP54936.exe"));


_LIT(KName,"CP54936");
const TUid KPluginUid={0x1028703b};


void TestOut(const TDesC16 &des)
	{
	for (TInt i=0; i<des.Length(); i++)
		test.Printf(_L("%04X "), des[i]);
	}

void TestOut(const TDesC8 &des)
	{
	for (TInt i=0; i<des.Length(); i++)
		test.Printf(_L("%02X "), des[i]);
	}

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
	
	// from Test Analysis:
	// p1:		0x40 -> 0x40
	// p2:		0x24 -> 0x24
	// p3:		0x706C -> 0xECE1
	// p4:		0x4E96 -> 0x8181
	// p5:		0x20AC -> 0xA2E3
	// p6:		0x3622 -> 0x8230A730, 0x060C -> 0x81318132
	// p7:		0x201AD (0xD840 0xDDAD) -> 0x9532AD35
	// p10-p15:	N/A
	// p20-p22: N/A
	// p30:		0x32FF -> 0x8139D633
	// p31:		0x10500 (0xD801 0xDD00) -> 0x90318330
	//			0xFFFF -> 0x8431A439
	_LIT16(Uni_1, "\x0040\xD840\x0024\x060C\x706C\x4E96\x20AC\x3622\xD840\xDDAD\x32FF\xD801\xDD00\xDC00\xFFFF");
	_LIT8(CP54936_1, "\x40\x5F\x24\x81\x31\x81\x32\xEC\xE1\x81\x81\xA2\xE3\x82\x30\xA7\x30\x95\x32\xAD\x35\x81\x39\xD6\x33\x90\x31\x83\x30\x5F\x84\x31\xA4\x39");

	TBuf8<200> foreign1;
	TBuf16<200> unicode2;
	
	const TDesC16& unicode1(Uni_1);
	test.Printf(_L("source: "));	TestOut(unicode1);	test.Printf(_L("\n"));
	test.Printf(_L("expect: "));	TestOut(CP54936_1);	test.Printf(_L("\n"));
	//TRAPD(err, (*aConvertFromUnicodeL)(foreign1, unicode1));
	(*aConvertFromUnicodeL)(foreign1, unicode1); 	//testing conversion from Unicode
	test.Printf(_L("result: "));	TestOut(foreign1);	test.Printf(_L("\n"));
	TInt error = foreign1.Compare(CP54936_1);
	test(error==0);
	foreign1.Zero();

	// from Test Analysis:
	// p1:		0x40
	// p2:		0x24
	// p3:		0x706C <- 0xECE1
	// p4:		0x4E96 <- 0x8181
	// p5:		0x20AC <- 0xA2E3
	// p6:		0x3622 <- 0x8230A730, 0x060C <- 0x81318132
	// p7:		0x201AD (0xD840 0xDDAD) <- 0x9532AD35
	// p10:		//0x0E -> 0xFFFD
	// p11:		//0xA0 -> 0xFFFD
	// p12:		0xA1A0 -> 0xE525
	// p13:		0xA07F -> 0xFFFD
	// p14:		0x95328230 -> 0x1FFFA (0xD83F 0xDFFA) //0xFFFD
	// p15:		0x8130813A -> 0xFFFD
	// p20-p22:	N/A
	// p30-p31:	N/A
	_LIT16(Uni_2, "\x0040\x0024\x706C\x4E96\x20AC\x3622\x060C\xD840\xDDAD\xE525\xFFFD\xD83F\xDFFA\xFFFD");
	_LIT8(CP54936_2, "\x40\x24\xEC\xE1\x81\x81\xA2\xE3\x82\x30\xA7\x30\x81\x31\x81\x32\x95\x32\xAD\x35\xA1\xA0\xA0\x7F\x95\x32\x82\x30\x81\x30\x81\x3A");

	const TDesC8& foreign2(CP54936_2);
	test.Printf(_L("source: "));	TestOut(foreign2);	test.Printf(_L("\n"));
	test.Printf(_L("expect: "));	TestOut(Uni_2);		test.Printf(_L("\n"));
	//TRAP(err, (*aConvertToUnicodeL)(unicode2,foreign2));
	(*aConvertToUnicodeL)(unicode2,foreign2);//); 	//testing conversion to Unicode
	test.Printf(_L("result: "));	TestOut(unicode2);	test.Printf(_L("\n"));
	error = unicode2.Compare(Uni_2);
	test(error==0);
	unicode2.Zero();
	
	
	//testing for legal short name character
	TInt result = (*aIsLegalShortNameCharacter)(0x005F); //testing for existent character
	test(result==1);
	result = (*aIsLegalShortNameCharacter)(0x003F); //testing for illegal character
	test(result==0);
	result = (*aIsLegalShortNameCharacter)(0x2999); 
	test(result==1);
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


const TInt MaxCount = 10000;	// iteration number for performance tests

// cp54936 should be faster than cp936
_LIT(KName936,"CP936");
const TUid KPluginUid936={0x10206A91};
void PerformanceTest1()
	{
	test.Next(_L("Performance test 1 (comparing with cp936)"));
	RLibrary lib936;
	RLibrary lib54936;

	const TUidType serverUid936(KNullUid,KNullUid,KPluginUid936);	
	const TUidType serverUid54936(KNullUid,KNullUid,KPluginUid);
	
	// load the dll	
	TInt returnValue = lib936.Load(KName936,serverUid936);
	test(returnValue==0);
	TInt returnValue2 = lib54936.Load(KName,serverUid54936);
	test(returnValue2==0);

	// get a pointer to the specified ordinal function and call it	
	TLibraryFunction function9361 = lib936.Lookup(1);
	TLibraryFunction function9362 = lib936.Lookup(2);
	TLibraryFunction function9363 = lib936.Lookup(3);
	TLibraryFunction function549361 = lib54936.Lookup(1);
	TLibraryFunction function549362 = lib54936.Lookup(2);
	TLibraryFunction function549363 = lib54936.Lookup(3);

	//cast the function pointer f to a function of type void with two arguments
	typedef void (*TConvertFromUnicodeL)(TDes8&, const TDesC16&);
	TConvertFromUnicodeL aConvertFromUnicode936L = reinterpret_cast <TConvertFromUnicodeL> (function9361);
	TConvertFromUnicodeL aConvertFromUnicode54936L = reinterpret_cast <TConvertFromUnicodeL> (function549361);
	
	typedef void (*TConvertToUnicodeL)(TDes16&, const TDesC8&);
	TConvertToUnicodeL aConvertToUnicode936L = reinterpret_cast <TConvertToUnicodeL> (function9362);
	TConvertToUnicodeL aConvertToUnicode54936L = reinterpret_cast <TConvertToUnicodeL> (function549362);

	// timer
	TInt count;
	TChar res2;
	TBuf<256> msg;
	TInt fastTimerFreq;
	HAL::Get(HALData::EFastCounterFrequency, fastTimerFreq);
	TReal ticksPerMicroSec = 1.0E-6 * fastTimerFreq;
	TUint prevTime;
	TUint timeDiff;
	TReal64 fsSessionMicroSecs;

	_LIT16(Uni_1, "\x0053\x0059\x004D\x3125\x3122\x9673\xFA29");
	_LIT8(CP54936_1, "\x53\x59\x4D\xA8\xE5\xA8\xE2\xEA\x90\xFE\x4F");
	TBuf8<20> foreign1;
	TBuf16<20> unicode2;
	const TDesC16& unicode1(Uni_1);
	TInt error;
	
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
	// 1, unicode -> 936
	// test result: 1448 ms for 100000 iterations
	test.Printf(_L("unicode --> 936:\n"));
	test.Printf(_L("        source: "));	TestOut(unicode1);	test.Printf(_L("\n"));
	test.Printf(_L("        expect: "));	TestOut(CP54936_1);	test.Printf(_L("\n"));
	prevTime = User::FastCounter();
	for (count=0; count<MaxCount; count++)
		{
		foreign1.Zero();
		(*aConvertFromUnicode936L)(foreign1, unicode1);
		}
	timeDiff = User::FastCounter() - prevTime;
	fsSessionMicroSecs = timeDiff / (ticksPerMicroSec);
	msg.Format(_L("                    %10.2lf us, "), fsSessionMicroSecs);
	test.Printf(_L("        result: "));	TestOut(foreign1);	test.Printf(_L("\n"));
	test.Printf(msg);
	error = foreign1.Compare(CP54936_1);
	test(error==0);
	foreign1.Zero();
	test.Printf(_L("\n"));
	
	// 2, unicode -> 54936
	// test result: 44 ms for 100000 iterations
	test.Printf(_L("unicode --> 54936:\n"));
	test.Printf(_L("        source: "));	TestOut(unicode1);	test.Printf(_L("\n"));
	test.Printf(_L("        expect: "));	TestOut(CP54936_1);	test.Printf(_L("\n"));
	prevTime = User::FastCounter();
	for (count=0; count<MaxCount; count++)
		{
		foreign1.Zero();
		(*aConvertFromUnicode54936L)(foreign1, unicode1);
		}
	timeDiff = User::FastCounter() - prevTime;
	fsSessionMicroSecs = timeDiff / (ticksPerMicroSec);
	msg.Format(_L("                    %10.2lf us, "), fsSessionMicroSecs);
	test.Printf(_L("        result: "));	TestOut(foreign1);	test.Printf(_L("\n"));
	test.Printf(msg);
	error = foreign1.Compare(CP54936_1);
	test(error==0);
	foreign1.Zero();
	test.Printf(_L("\n"));

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
	_LIT16(Uni_2, "\x0032\xFFFD\x7FB1\x0032\xFFFD\x7FB1\x7FB1");
	_LIT8(CP54936_2, "\x32\xC1\x7F\xC1\x7E\x32\xC1\x7F\xC1\x7E\xC1\x7E");
	const TDesC8& foreign2(CP54936_2);

	// 3, 936 -> unicode
	// test result: 89 ms for 100000 iterations
	test.Printf(_L("936 ---> unicode:\n"));
	test.Printf(_L("        source: "));	TestOut(foreign2);	test.Printf(_L("\n"));
	test.Printf(_L("        expect: "));	TestOut(Uni_2);		test.Printf(_L("\n"));
	prevTime = User::FastCounter();
	for (count=0; count<MaxCount; count++)
		{
		unicode2.Zero();
		(*aConvertToUnicode936L)(unicode2,foreign2);
		}
	timeDiff = User::FastCounter() - prevTime;
	fsSessionMicroSecs = timeDiff / (ticksPerMicroSec);
	msg.Format(_L("                    %10.2lf us, "), fsSessionMicroSecs);
	test.Printf(_L("        result: "));	TestOut(unicode2);	test.Printf(_L("\n"));
	test.Printf(msg);
	error = unicode2.Compare(Uni_2);
	test(error==0);
	unicode2.Zero();
	test.Printf(_L("\n"));

	// 4, 54936 -> unicode
	// test result: 36 ms for 100000 iterations
	test.Printf(_L("54936 ---> unicode:\n"));
	test.Printf(_L("        source: "));	TestOut(foreign2);	test.Printf(_L("\n"));
	test.Printf(_L("        expect: "));	TestOut(Uni_2);		test.Printf(_L("\n"));
	prevTime = User::FastCounter();
	for (count=0; count<MaxCount; count++)
		{
		unicode2.Zero();
		(*aConvertToUnicode54936L)(unicode2,foreign2);
		}
	timeDiff = User::FastCounter() - prevTime;
	fsSessionMicroSecs = timeDiff / (ticksPerMicroSec);
	msg.Format(_L("                    %10.2lf us, "), fsSessionMicroSecs);
	test.Printf(_L("        result: "));	TestOut(unicode2);	test.Printf(_L("\n"));
	test.Printf(msg);
	error = unicode2.Compare(Uni_2);
	test(error==0);
	unicode2.Zero();
	test.Printf(_L("\n"));
	
	lib936.Close();
	lib54936.Close();
	
//	test.Printf(_L("Press any key...\n"));
//	test.Getch();
	}


// performance when converting mixed data.
// just for reference.
// ideally, the test result should be similar to that in PerformanceTest1()
void PerformanceTest2()
	{
	test.Next(_L("Performance test 2 (reference)"));
	RLibrary lib54936;
	const TUidType serverUid54936(KNullUid,KNullUid,KPluginUid);
	
	// load the dll	
	TInt returnValue2 = lib54936.Load(KName,serverUid54936);
	test(returnValue2==0);

	// get a pointer to the specified ordinal function and call it	
	TLibraryFunction function549361 = lib54936.Lookup(1);
	TLibraryFunction function549362 = lib54936.Lookup(2);
	TLibraryFunction function549363 = lib54936.Lookup(3);

	//cast the function pointer f to a function of type void with two arguments
	typedef void (*TConvertFromUnicodeL)(TDes8&, const TDesC16&);
	TConvertFromUnicodeL aConvertFromUnicode54936L = reinterpret_cast <TConvertFromUnicodeL> (function549361);
	
	typedef void (*TConvertToUnicodeL)(TDes16&, const TDesC8&);
	TConvertToUnicodeL aConvertToUnicode54936L = reinterpret_cast <TConvertToUnicodeL> (function549362);

	// timer
	TInt count;
	TBuf<256> msg;
	TInt fastTimerFreq;
	HAL::Get(HALData::EFastCounterFrequency, fastTimerFreq);
	TReal ticksPerMicroSec = 1.0E-6 * fastTimerFreq;
	TUint prevTime;
	TUint timeDiff;
	TReal64 fsSessionMicroSecs;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
	_LIT16(Uni_1, "\x0053\x0059\x004D\x3125\x3122\x9673\xFA29\x060C\xD840\xDC00");
	_LIT8(CP54936_1, "\x53\x59\x4D\xA8\xE5\xA8\xE2\xEA\x90\xFE\x4F\x81\x31\x81\x32\x95\x32\x82\x36");
	TBuf8<200> foreign1;
	TBuf16<200> unicode2;
	const TDesC16& unicode1(Uni_1);
	TInt error;

	// 2, unicode -> 54936
	// test result: 130 ms for 100000 iterations (44 ms if one huge table for BMP characters)
	test.Printf(_L("unicode --> 54936:\n"));
	test.Printf(_L("        source: "));	TestOut(unicode1);	test.Printf(_L("\n"));
	test.Printf(_L("        expect: "));	TestOut(CP54936_1);	test.Printf(_L("\n"));
	prevTime = User::FastCounter();
	for (count=0; count<MaxCount; count++)
		{
		foreign1.Zero();
		(*aConvertFromUnicode54936L)(foreign1, unicode1);
		}
	timeDiff = User::FastCounter() - prevTime;
	fsSessionMicroSecs = timeDiff / (ticksPerMicroSec);
	msg.Format(_L("                    %10.2lf us, "), fsSessionMicroSecs);
	test.Printf(_L("        result: "));	TestOut(foreign1);	test.Printf(_L("\n"));
	test.Printf(msg);
	error = foreign1.Compare(CP54936_1);
	test(error==0);
	foreign1.Zero();
	test.Printf(_L("\n"));

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
	_LIT16(Uni_2, "\x0032\x20AC\xFFFD\x7FB1\x47BB\xD840\xDC00");
	_LIT8(CP54936_2, "\x32\xA2\xE3\xC1\x7F\xC1\x7E\x82\x33\xED\x33\x95\x32\x82\x36");
	const TDesC8& foreign2(CP54936_2);

	// 4, 54936 -> unicode
	// test result: 36 ms for 100000 iterations
	test.Printf(_L("54936 ---> unicode:\n"));
	test.Printf(_L("        source: "));	TestOut(foreign2);	test.Printf(_L("\n"));
	test.Printf(_L("        expect: "));	TestOut(Uni_2);		test.Printf(_L("\n"));
	prevTime = User::FastCounter();
	for (count=0; count<MaxCount; count++)
		{
		unicode2.Zero();
		(*aConvertToUnicode54936L)(unicode2,foreign2);
		}
	timeDiff = User::FastCounter() - prevTime;
	fsSessionMicroSecs = timeDiff / (ticksPerMicroSec);
	msg.Format(_L("                    %10.2lf us, "), fsSessionMicroSecs);
	test.Printf(_L("        result: "));	TestOut(unicode2);	test.Printf(_L("\n"));
	test.Printf(msg);
	error = unicode2.Compare(Uni_2);
	test(error==0);
	unicode2.Zero();
	test.Printf(_L("\n"));
	
	lib54936.Close();
	
//	test.Printf(_L("Press any key...\n"));
//	test.Getch();
	}

/**
@SYMTestCaseID          TI18N-FATCHARCONV-CIT-4001
@SYMTestCaseDesc        Check GB18030 support 
@SYMTestPriority        High
@SYMTestActions         1. Get one-byte Unicode codepoint
                        2. Convert to/from GB18030 to/from Unicode
                        3. Get two-byte Unicode codepoint
                        4. Convert to/from GB18030 to/from Unicode
                        5. Get four-byte Unicode codepoint
                        6. Convert to/from GB18030 to/from Unicode
@SYMTestExpectedResults No side effect
@SYMREQ                 REQ12067
*/
LOCAL_C void TestConversion( const TDesC16& aUnicode, const TDesC8& a54936, TBool aZero1=ETrue, TBool aZero2=ETrue )
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
	
	//	testing conversion from Unicode
	TBuf8<200> generated54936;
	test.Printf(_L("source: "));	
	TestOut(aUnicode);	
	test.Printf(_L("\n"));
	test.Printf(_L("expect: "));	
	TestOut(a54936);	
	test.Printf(_L("\n"));
	(*aConvertFromUnicodeL)(generated54936, aUnicode); 	
	test.Printf(_L("result: "));	
	TestOut(generated54936);	
	test.Printf(_L("\n"));
	TInt error = generated54936.Compare(a54936);
	if ( aZero1 )
	{
		test(error==0);
	}
	else
	{
		test(error!=0);
	}

	//	testing conversion to Unicode
	TBuf16<200> generatedUnicode;
	test.Printf(_L("source: "));	
	TestOut(a54936);	
	test.Printf(_L("\n"));
	test.Printf(_L("expect: "));	
	TestOut(aUnicode);		
	test.Printf(_L("\n"));
	(*aConvertToUnicodeL)(generatedUnicode,a54936);
	test.Printf(_L("result: "));	
	TestOut(generatedUnicode);	
	test.Printf(_L("\n"));
	error = generatedUnicode.Compare(aUnicode);
	if ( aZero2 )
	{
		test(error==0);
	}
	else
	{
		test(error!=0);
	}
	
	lib.Close();
}


/**
@SYMTestCaseID          TI18N-CHARCONV-CT-
@SYMTestCaseDesc        Check GB18030 support 
@SYMTestPriority        High
@SYMTestActions         1. Get one-byte Unicode codepoint
						2. Convert to/from GB18030 to/from Unicode
						3. Get two-byte Unicode codepoint
						4. Convert to/from GB18030 to/from Unicode
						5. Get four-byte Unicode codepoint
						6. Convert to/from GB18030 to/from Unicode
@SYMTestExpectedResults No side effect
@SYMREQ                 REQ12067
*/
LOCAL_C void TestGbConversion()
{
	//	Test() function covers GB 1,2,4 bytes
	//	one-byte
	//	border 0x80
	_LIT16(Uni_0, "\x0000");
	_LIT8(CP54936_0, "\x00");
	TestConversion( Uni_0, CP54936_0 );		

	_LIT16(Uni_1, "\x0079");
	_LIT8(CP54936_1, "\x79");
	TestConversion( Uni_1, CP54936_1 );		

	_LIT16(Uni_2, "\x0080");
	_LIT8(CP54936_2, "\x81\x30\x81\x30");
	TestConversion( Uni_2, CP54936_2 );		

	_LIT16(Uni_3, "\x0081");
	_LIT8(CP54936_3, "\x81\x30\x81\x31");
	TestConversion( Uni_3, CP54936_3 );		
	
	_LIT16(Uni_4, "\x00fe");
	_LIT8(CP54936_4, "\x81\x30\x8B\x36");
	TestConversion( Uni_4, CP54936_4 );		

	_LIT16(Uni_5, "\x00ff");
	_LIT8(CP54936_5, "\x81\x30\x8B\x37");
	TestConversion( Uni_5, CP54936_5 );		

	//	two-byte
	_LIT16(Uni_6, "\x0100");
	_LIT8(CP54936_6, "\x81\x30\x8B\x38");
	TestConversion( Uni_6, CP54936_6 );	

	_LIT16(Uni_7, "\x0101");
	_LIT8(CP54936_7, "\xA8\xA1");
	TestConversion( Uni_7, CP54936_7 );	

	_LIT16(Uni_8, "\x0ffe");
	_LIT8(CP54936_8, "\x81\x33\x83\x38");
	TestConversion( Uni_8, CP54936_8 );	

	_LIT16(Uni_9, "\x0fff");
	_LIT8(CP54936_9, "\x81\x33\x83\x39");
	TestConversion( Uni_9, CP54936_9 );	

	_LIT16(Uni_10, "\x1000");
	_LIT8(CP54936_10, "\x81\x33\x84\x30");
	TestConversion( Uni_10, CP54936_10 );	

	_LIT16(Uni_11, "\x1001");
	_LIT8(CP54936_11, "\x81\x33\x84\x31");
	TestConversion( Uni_11, CP54936_11 );	

	_LIT16(Uni_12, "\xfffe");
	_LIT8(CP54936_12, "\x84\x31\xA4\x38");
	TestConversion( Uni_12, CP54936_12 );	

	_LIT16(Uni_13, "\xffff");
	_LIT8(CP54936_13, "\x84\x31\xA4\x39");
	TestConversion( Uni_13, CP54936_13 );	

	//	four-byte
	_LIT16(Uni_14, "\xd840\xdc00");
	_LIT8(CP54936_14, "\x95\x32\x82\x36");
	TestConversion( Uni_14, CP54936_14 );	

	_LIT16(Uni_15, "\xd840\xdc01");
	_LIT8(CP54936_15, "\x95\x32\x82\x37");
	TestConversion( Uni_15, CP54936_15 );	
	
	_LIT16(Uni_16, "\xD87F\xdffe");
	_LIT8(CP54936_16, "\x9a\x34\x84\x30");
	TestConversion( Uni_16, CP54936_16 );	

	_LIT16(Uni_17, "\xD87F\xdfff");
	_LIT8(CP54936_17, "\x9a\x34\x84\x31");
	TestConversion( Uni_17, CP54936_17 );	

	//	4-byte gb
	_LIT16(Uni_18, "\xd840\xddad");
	_LIT8(CP54936_18, "\x95\x32\xAD\x35");
	TestConversion( Uni_18, CP54936_18 );	
	
	_LIT16(Uni_19, "\xd801\xdd00");
	_LIT8(CP54936_19, "\x90\x31\x83\x30");
	TestConversion( Uni_19, CP54936_19 );	
}


/**
@SYMTestCaseID          TI18N-CHARCONV-CT-
@SYMTestCaseDesc        Test FAT short name legal character
@SYMTestPriority        High
@SYMTestActions         1. Get one-byte Unicode codepoint
						2. Check if it's legal short name character
						3. Get two-byte Unicode codepoint
						4. Check if it's legal short name character
						5. Get four-byte Unicode codepoint
						6. Check if it's legal short name character
@SYMTestExpectedResults No side effect
@SYMREQ                 REQ12067
*/
LOCAL_C void TestShortNameCharacter()
{
	test.Next(_L(" @SYMTestCaseID:SYSLIB-FATCHARSETCONV-CT-1778 "));
	RLibrary lib;

	const TUidType serverUid(KNullUid,KNullUid,KPluginUid);	
	// 	load the dll	
	TInt returnValue = lib.Load(KName,serverUid);
	test(returnValue==0);

	// 	get a pointer to the specified ordinal function and call it	
	TLibraryFunction function1 = lib.Lookup(1);
	TLibraryFunction function2 = lib.Lookup(2);
	TLibraryFunction function3 = lib.Lookup(3);

	typedef TBool (*TIsLegalShortNameCharacter)(TUint);	
	TIsLegalShortNameCharacter aIsLegalShortNameCharacter = reinterpret_cast <TIsLegalShortNameCharacter> (function3);
		
	//	testing for legal short name character
	//	one-byte unicode
	//	0x20 is space
	TInt result = (*aIsLegalShortNameCharacter)(0x0019); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x0020); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x0021); 
	test(result==1);

	result = (*aIsLegalShortNameCharacter)(0x005F); 
	test(result==1);

	//	0x80 is ascii border
	result = (*aIsLegalShortNameCharacter)(0x0079); 
	test(result==1);

	result = (*aIsLegalShortNameCharacter)(0x0080); 
	test(result==1);

	result = (*aIsLegalShortNameCharacter)(0x0081); 
	test(result==1);

	//	testing for illegal character
	result = (*aIsLegalShortNameCharacter)(0x003F); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x22); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x2A); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x2B); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x2C); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x2F); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x3A); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x3B); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x3C); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x3D); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x3E); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x5B); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x5C); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x5D); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0x7C); 
	test(result==0);

	//	two-byte unicode
	result = (*aIsLegalShortNameCharacter)(0x1000); 
	test(result==1);

	result = (*aIsLegalShortNameCharacter)(0x1001); 
	test(result==1);

	result = (*aIsLegalShortNameCharacter)(0x2999); //testing for non-existent character
	test(result==1);

	result = (*aIsLegalShortNameCharacter)(0x4E02); //testing for a double byte character
	test(result==1);

	result = (*aIsLegalShortNameCharacter)(0xfffe); 
	test(result==1);

	result = (*aIsLegalShortNameCharacter)(0xffff); 
	test(result==1);

	result = (*aIsLegalShortNameCharacter)(0xd7ff); 
	test(result==1);

	//	surrogate part
	result = (*aIsLegalShortNameCharacter)(0xd800); 
	test(result==0);
	
	result = (*aIsLegalShortNameCharacter)(0xdfff); 
	test(result==0);

	result = (*aIsLegalShortNameCharacter)(0xe000); 
	test(result==1);

	//	four-byte unicode
	result = (*aIsLegalShortNameCharacter)(0x10000); 
	test(result==1);

	result = (*aIsLegalShortNameCharacter)(0x10001); 
	test(result==1);

	result = (*aIsLegalShortNameCharacter)(0x10fffe); 
	test(result==1);

	result = (*aIsLegalShortNameCharacter)(0x10ffff); 
	test(result==1);

	result = (*aIsLegalShortNameCharacter)(0x110000); 
	test(result==0);

	lib.Close();
}


LOCAL_C void DoE32MainL()
	{
	__UHEAP_MARK;
	
	TestGbConversion();
	TestShortNameCharacter();
	Test();
	TestINC090073();
	OOMTest();
	PerformanceTest1();
	PerformanceTest2();
	
	__UHEAP_MARKEND;
	}

GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;

	test.Title();
	test.Start(_L("CP54936 test..."));

	CTrapCleanup* trapCleanup=CTrapCleanup::New();
	TRAPD(error, DoE32MainL());
	test(error==KErrNone);
	
	delete trapCleanup;
	
	test.End();
	test.Close();

	__UHEAP_MARKEND;
	return error;
	}
