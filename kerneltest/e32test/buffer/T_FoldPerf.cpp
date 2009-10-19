// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32test.h>

static RTest TheTest(_L("T_FoldPerf"));

#define ARRAY_SIZE(ar) (sizeof(ar) / (sizeof(ar[0])))

const TText16 KText16[] = {'1', 'f', 'A', 0x01D5, 'k', '5', 'g', 'U', 'W', 'q', 'a', 
                           0x095E, 0x01D5, 'a', 'B', 'c', 'd', 'E', 'F', 0};
//0x095E - DEVANAGARI LETTER FA
//decomposed to: 0x092B 0x093C
//0x01D5 - LATIN CAPITAL LETTER U WITH DIAERESIS AND MACRON
//decomposed to: 0x0075 0x0308 0x0304

_LIT16(KPlainText, "abcdefghijklmnopqrst");

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-0108
@SYMTestCaseDesc FindF - performance test
@SYMTestPriority High
@SYMTestActions  FindF - performance test
@SYMTestExpectedResults The test must not fail.
@SYMPREQ814 Optimise folded string comparisons.
*/
static void FindF_PerformanceTest()
    {
	TheTest.Next(_L("FindF test"));

    TBufC16<ARRAY_SIZE(KText16) - 1> text(KText16);

    TBufC16<3> searchStr1(_L16("gUw"));

    const TText16 searchText2[] = {0x01D5, 'A', 'b', 0};
    TBufC16<ARRAY_SIZE(searchText2) - 1> searchStr2(searchText2);

    const TText16 searchText3[] = {0x0075, 0x0308, 0x0304, 'A', 'b', 0};
    TBufC16<ARRAY_SIZE(searchText3) - 1> searchStr3(searchText3);

    const TText16 searchText4[] = {0x095E, 'd', 'A', 'b', 0};
    TBufC16<ARRAY_SIZE(searchText4) - 1> searchStr4(searchText4);

    const TText16 searchText5[] = {0x095E, 0x0055, 0x0308, 0x0304, 'A', 'b', 'C', 0};
    TBufC16<ARRAY_SIZE(searchText5) - 1> searchStr5(searchText5);

    TBufC16<4> searchStr6(_L16("CDEF"));

    TUint timeStart = User::TickCount();
    for(TInt i=0;i<10000;++i)
        {
        TInt res = text.FindF(searchStr1);
        TheTest(res == 6);//searchStr1 starts at position 6

        res = text.FindF(searchStr2);
        TheTest(res == 12);

        res = text.FindF(searchStr3);
        TheTest(res == 12);

        res = text.FindF(searchStr4);
        TheTest(res == KErrNotFound);

        res = text.FindF(searchStr5);
        TheTest(res == 11);

        res = text.FindF(searchStr6);
        TheTest(res == 15);
        }
	TUint timeEnd = User::TickCount();
	TheTest.Printf(_L("Time = %d ticks\n"), timeEnd - timeStart);
    }

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-0109
@SYMTestCaseDesc MatchF - performance test
@SYMTestPriority High
@SYMTestActions  MatchF - performance test
@SYMTestExpectedResults The test must not fail.
@SYMPREQ814 Optimise folded string comparisons.
*/
static void MatchF_PerformanceTest()
    {
	TheTest.Next(_L("MatchF test"));

    TBufC16<ARRAY_SIZE(KText16) - 1> text(KText16);

    TBufC16<11> searchStr1(_L16("*fa??5*W*a*"));

    const TText16 searchText2[] = {'*', 0x01D5, 'A', '?', 'C', '*', 0};
    TBufC16<ARRAY_SIZE(searchText2) - 1> searchStr2(searchText2);

    const TText16 searchText3[] = {'*', 0x0075, 0x0308, 0x0304, '*', 'e', 'F', 0};
    TBufC16<ARRAY_SIZE(searchText3) - 1> searchStr3(searchText3);

    const TText16 searchText4[] = {'1', '*', 'A', 'b', '*', 0};
    TBufC16<ARRAY_SIZE(searchText4) - 1> searchStr4(searchText4);

    TBufC16<11> searchStr5(_L16("*fa?z5*W*a*"));

    TBufC16<5> searchStr6(_L16("a?v?T"));

    TUint timeStart = User::TickCount();
    for(TInt i=0;i<10000;++i)
        {
        TInt res = text.MatchF(searchStr1);
        TheTest(res == 1);

        res = text.MatchF(searchStr2);
        TheTest(res == 12);

        res = text.MatchF(searchStr3);
        TheTest(res == 3);

        res = text.MatchF(searchStr4);
        TheTest(res == 0);

        res = text.MatchF(searchStr5);
        TheTest(res == KErrNotFound);

        res = text.MatchF(searchStr6);
        TheTest(res == KErrNotFound);
        }
	TUint timeEnd = User::TickCount();
	TheTest.Printf(_L("Time = %d ticks\n"), timeEnd - timeStart);
    }

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-0110
@SYMTestCaseDesc CompareF - performance test
@SYMTestPriority High
@SYMTestActions  CompareF - performance test
@SYMTestExpectedResults The test must not fail.
@SYMPREQ814 Optimise folded string comparisons.
*/
static void CompareF_PerformanceTest()
    {
	TheTest.Next(_L("CompareF test"));

    TBufC16<ARRAY_SIZE(KText16) - 1> text(KText16);

    const TText16 text1[] = {'1', 'F', 'A', 0x01D5, 'k', '5', 'G', 'U', 'W', 'Q', 'A', 
                             0x095E, 0x01D5, 'a', 'B', 'C', 'd', 'E', 'F', 0};
    TBufC16<ARRAY_SIZE(text1) - 1> str1(text1);

    TBufC16<19> str2(_L16("1234567890123456789"));

    TBufC16<19> str3(_L16("1fA4G6789r1d34z6789"));

    TUint timeStart = User::TickCount();
    TInt i;
    for(i=0;i<10000;++i)
        {
        TInt res = text.CompareF(str1);
        TheTest(res == 0);

        res = text.CompareF(str2);
        TheTest(res != 0);

        res = text.CompareF(str3);
        TheTest(res != 0);
        }
	TUint timeEnd = User::TickCount();
	TheTest.Printf(_L("1. Time = %d ticks\n"), timeEnd - timeStart);

    TBufC16<20> str4(_L16("abc3456hijklmnopqrst"));
    timeStart = User::TickCount();
    for(i=0;i<10000;++i)
        {
        TInt res = str4.CompareF(KPlainText);
        TheTest(res != 0);
        }
	timeEnd = User::TickCount();
	TheTest.Printf(_L("2. Time = %d ticks\n"), timeEnd - timeStart);

    }

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-0114
@SYMTestCaseDesc FindF(), CompareF(), MatchF() on plain (no combining characters) text
@SYMTestPriority High
@SYMTestActions  Performance test
@SYMTestExpectedResults The test must not fail.
@SYMPREQ814 Optimise folded string comparisons.
*/
static void PlainTextPerformanceTest()
    {
	TheTest.Next(_L("Plain text - performance test"));

    TInt res;
    TInt i;

    TBufC16<20> str1(_L16("abcdefghijklmnopqrst"));
    TBufC16<20> str2(_L16("abcDefghIjklmNOpqrsT"));
    TBufC16<20> str3(_L16("abcDefghIjKlmNOp2rsT"));

    TUint timeStart = User::TickCount();
    for(i=0;i<10000;++i)
        {
        res = KPlainText().CompareF(str1);
        TheTest(res == 0);
        res = KPlainText().CompareF(str2);
        TheTest(res == 0);
        res = KPlainText().CompareF(str3);
        TheTest(res != 0);
        }
	TUint timeEnd = User::TickCount();
	TheTest.Printf(_L("CompareF() Time = %d ticks\n"), timeEnd - timeStart);

    TBufC16<20> str4(_L16("gHiJk"));
    TBufC16<20> str5(_L16("Opqr"));
    TBufC16<20> str6(_L16("2rsT"));

    timeStart = User::TickCount();
    for(i=0;i<10000;++i)
        {
        res = KPlainText().FindF(str4);
        TheTest(res == 6);
        res = KPlainText().FindF(str5);
        TheTest(res == 14);
        res = KPlainText().FindF(str6);
        TheTest(res == KErrNotFound);
        }
	timeEnd = User::TickCount();
	TheTest.Printf(_L("FindF() Time = %d ticks\n"), timeEnd - timeStart);

    TBufC16<20> str7(_L16("*gHiJk*op??sT"));
    TBufC16<20> str8(_L16("aBC*rst"));
    TBufC16<20> str9(_L16("ab?D*2rsT"));

    timeStart = User::TickCount();
    for(i=0;i<10000;++i)
        {
        res = KPlainText().MatchF(str7);
        TheTest(res == 6);
        res = KPlainText().MatchF(str8);
        TheTest(res == 0);
        res = KPlainText().MatchF(str9);
        TheTest(res == KErrNotFound);
        }
	timeEnd = User::TickCount();
	TheTest.Printf(_L("MatchF() Time = %d ticks\n"), timeEnd - timeStart);

    TBufC16<21> str10(_L16("abcdefghijklmnopqrst"));
    TBufC16<20> str11(_L16("*xyz*"));
    TBufC16<20> str12(_L16("xyz"));

    timeStart = User::TickCount();
    for(i=0;i<10000;++i)
        {
        res = str10.FindF(str12);
        TheTest(res == KErrNotFound);
        }
	timeEnd = User::TickCount();
	TheTest.Printf(_L("Nonmatching string. FindF() Time = %d ticks\n"), timeEnd - timeStart);

    timeStart = User::TickCount();
    for(i=0;i<10000;++i)
        {
        res = str10.MatchF(str11);
        TheTest(res == KErrNotFound);
        res = str10.MatchF(str12);
        TheTest(res == KErrNotFound);
        }
	timeEnd = User::TickCount();
	TheTest.Printf(_L("Nonmatching string. MatchF() Time = %d ticks\n"), timeEnd - timeStart);
    }
    
static void RunComparison_PerformanceTest(TInt aNumberOfTimes, TDesC& aOriginalText, TDesC& aToCompareText, TBool aCase, TInt aValue = 0)
	{
	TUint timeStart;
	TUint timeEnd;
	TInt res = 0;
	TInt i;
	
	// CompareF() case...
	timeStart = User::TickCount();
	
	for(i=0; i<aNumberOfTimes; ++i)
        {
        res = aOriginalText.CompareF(aToCompareText);
        
        if(aCase)
        	TheTest(res == 0);
        else
        	TheTest(res != 0);
        }
 
    timeEnd = User::TickCount();
    
    TheTest.Printf(_L("Time to run (x%d) 16-bit CompareF() = %d ticks\n"), aNumberOfTimes, timeEnd - timeStart);
    
    // Check return value
    
    TheTest(res == aValue);
	
	// Compare() case...
	timeStart = User::TickCount();
	
	for(i=0; i<aNumberOfTimes; ++i)
        {
        res = aOriginalText.Compare(aToCompareText);
        
        if(aCase)
        	TheTest(res == 0);
        else
        	TheTest(res != 0);
        }
        
    timeEnd = User::TickCount();
    
    TheTest.Printf(_L("Time to run (x%d) 16-bit Compare() = %d ticks\n"), aNumberOfTimes, timeEnd - timeStart);
	}
    
static void RunComparison_PerformanceTest(TInt aNumberOfTimes, TDesC8& aOriginalText, TDesC8& aToCompareText, TBool aCase, TInt aValue = 0)
	{
	TUint timeStart;
	TUint timeEnd;
	TInt res = 0;
	TInt i;
	
	// CompareF() case...
	timeStart = User::TickCount();
	
	for(i=0; i<aNumberOfTimes; ++i)
        {
        res = aOriginalText.CompareF(aToCompareText);
        
        if(aCase)
        	TheTest(res == 0);
        else
        	TheTest(res != 0);
        }
 
    timeEnd = User::TickCount();
    
    TheTest.Printf(_L("Time to run (x%d) 8-bit CompareF() = %d ticks\n"), aNumberOfTimes, timeEnd - timeStart);

    // Check return value
    
    TheTest(res == aValue);
	
	// Compare() case...
	timeStart = User::TickCount();
	
	for(i=0; i<aNumberOfTimes; ++i)
        {
        res = aOriginalText.Compare(aToCompareText);
        
        if(aCase)
        	TheTest(res == 0);
        else
        	TheTest(res != 0);
        }
        
    timeEnd = User::TickCount();
    
    TheTest.Printf(_L("Time to run (x%d) 8-bit Compare() = %d ticks\n"), aNumberOfTimes, timeEnd - timeStart);
	}  
  
static void CompareVsCompareF_PerformanceTest()
	{
	TheTest.Next(_L("Compare verses CompareF test"));

	// Declare variables...
	
	TInt numberOfTimes = 100000;

	const TText16 smallUnicodeText16[] = {0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 0}; // 5 Characters
	
	const TText16 largeUnicodeText16[] = {0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
						   		 		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 0};	// 50 Characters
	
	const TText16 smallErrUnicodeTextLast16[] = {0x01D5, 0x0308, 0x0304, 0x095E, 'X', 0}; // 5 Characters
	
	const TText16 smallErrUnicodeTextFirst16[] = {'X', 0x0308, 0x0304, 0x095E, 0x01D5, 0}; // 5 Characters
	
	const TText16 largeErrUnicodeTextLast16[] = {0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
						   		 		  		 0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		 0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		 0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		 0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		 0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		 0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		 0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		 0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		 0x01D5, 0x0308, 0x0304, 0x095E, 'X', 0};	// 50 Characters
	
	const TText16 largeErrUnicodeTextFirst16[] = {'X', 0x0308, 0x0304, 0x095E, 0x01D5, 
						   		 		  		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 
								 		  		  0x01D5, 0x0308, 0x0304, 0x095E, 0x01D5, 0};	// 50 Characters
	
	const TText16 smallMixedText16[] = {'A', 'B', 'C', 0x095E, 0x01D5,  0}; // 5 Characters
	
	const TText16 largeMixedText16[] = {'A', 'B', 'C', 0x095E, 0x01D5, 
						   		 		'F', 'G', 'H', 'I', 'J', 
								 		'K', 'L', 'M', 'N', 'O', 
								 		'P', 'Q', 'R', 'S', 'T', 
								 		'U', 'V', 'W', 'X', 'Y', 
								 		'Z', '1', '2', '3', '4', 
								 		'5', '6', '7', '8', '9', 
								 		'a', 'b', 'c', 'd', 'e', 
								 		'f', 'g', 'h', 'i', 'j', 
								 		'k', 'l', 'm', 0x095E, 0x01D5, 0}; // 50 Characters
								 		
	const TText16 smallErrMixedTextLast16[] = {'A', 'B', 'C', 0x095E, 0x0304,  0}; // 5 Characters
	
	const TText16 smallErrMixedTextFirst16[] = {0x0304, 'B', 'C', 0x095E, 0x01D5,  0}; // 5 Characters
	
	const TText16 largeErrMixedTextLast16[] = {'A', 'B', 'C', 0x095E, 0x01D5, 
						   		 			   'F', 'G', 'H', 'I', 'J', 
								 			   'K', 'L', 'M', 'N', 'O', 
								 			   'P', 'Q', 'R', 'S', 'T', 
								 			   'U', 'V', 'W', 'X', 'Y', 
								 			   'Z', '1', '2', '3', '4', 
								 			   '5', '6', '7', '8', '9', 
								 			   'a', 'b', 'c', 'd', 'e', 
								 			   'f', 'g', 'h', 'i', 'j', 
								 			   'k', 'l', 'm', 0x095E, 0x0304, 0}; // 50 Characters
	
	const TText16 largeErrMixedTextFirst16[] = {0x0304, 'B', 'C', 0x095E, 0x01D5, 
						   		 				'F', 'G', 'H', 'I', 'J', 
						   		 				'K', 'L', 'M', 'N', 'O', 
						   		 				'P', 'Q', 'R', 'S', 'T', 
						   		 				'U', 'V', 'W', 'X', 'Y', 
						   		 				'Z', '1', '2', '3', '4', 
						   		 				'5', '6', '7', '8', '9', 
						   		 				'a', 'b', 'c', 'd', 'e', 
						   		 				'f', 'g', 'h', 'i', 'j', 
						   		 				'k', 'l', 'm', 0x095E, 0x01D5, 0}; // 50 Characters
	
	// (16-bit) Ascii Set of variables
	
	TBufC16<50> oriAsciiLargeText(_L("ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789abcdefghijklmno")); 
	TBufC16<5> oriAsciiSmallText(_L("ABCDE"));
	
	TBufC16<50> matchAsciiLargeText(_L("ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789abcdefghijklmno"));
	TBufC16<5> matchAsciiSmallText(_L("ABCDE"));
		
	TBufC16<50> nonMatchAsciiLargeTextLast(_L("ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789abcdefghijklmnX"));
	TBufC16<50> nonMatchAsciiLargeTextFirst(_L("XBCDEFGHIJKLMNOPQRSTUVWXYZ123456789abcdefghijklmno"));
	
	TBufC16<5> nonMatchAsciiSmallTextLast(_L("ABCDX"));
	TBufC16<5> nonMatchAsciiSmallTextFirst(_L("XBCDE"));
	
	// (8-bit) Ascii Set of variables
	
	TBufC8<50> oriAsciiLargeText8(_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789abcdefghijklmno"));
	TBufC8<5> oriAsciiSmallText8(_L8("ABCDE"));
	
	TBufC8<50> matchAsciiLargeText8(_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789abcdefghijklmno"));
	TBufC8<5> matchAsciiSmallText8(_L8("ABCDE"));
	
	TBufC8<50> nonMatchAsciiLargeTextLast8(_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789abcdefghijklmnX"));
	TBufC8<50> nonMatchAsciiLargeTextFirst8(_L8("XBCDEFGHIJKLMNOPQRSTUVWXYZ123456789abcdefghijklmno"));
	
	TBufC8<5> nonMatchAsciiSmallTextLast8(_L8("ABCDX"));
	TBufC8<5> nonMatchAsciiSmallTextFirst8(_L8("XBCDE"));
	
	// (16-bit) Unicode Set of variables
	
	TBufC16<ARRAY_SIZE(largeUnicodeText16) - 1> oriUnicodeLargeText(largeUnicodeText16);
	TBufC16<ARRAY_SIZE(smallUnicodeText16) - 1> oriUnicodeSmallText(smallUnicodeText16);
	
	TBufC16<ARRAY_SIZE(largeUnicodeText16) - 1> matchUnicodeLargeText(largeUnicodeText16);
	TBufC16<ARRAY_SIZE(smallUnicodeText16) - 1> matchUnicodeSmallText(smallUnicodeText16);
	
	TBufC16<ARRAY_SIZE(largeErrUnicodeTextLast16) - 1> nonMatchUnicodeLargeTextLast(largeErrUnicodeTextLast16);
	TBufC16<ARRAY_SIZE(largeErrUnicodeTextFirst16) - 1> nonMatchUnicodeLargeTextFirst(largeErrUnicodeTextFirst16);
	
	TBufC16<ARRAY_SIZE(smallErrUnicodeTextLast16) - 1> nonMatchUnicodeSmallTextLast(smallErrUnicodeTextLast16);
	TBufC16<ARRAY_SIZE(smallErrUnicodeTextFirst16) - 1> nonMatchUnicodeSmallTextFirst(smallErrUnicodeTextFirst16);
	
	// (16-bit) Unicode/Ascii Set of variables
	
	TBufC16<ARRAY_SIZE(largeMixedText16) - 1> oriMixedLargeText(largeMixedText16);
	TBufC16<ARRAY_SIZE(smallMixedText16) - 1> oriMixedSmallText(smallMixedText16);
	
	TBufC16<ARRAY_SIZE(largeMixedText16) - 1> matchMixedLargeText(largeMixedText16);
	TBufC16<ARRAY_SIZE(smallMixedText16) - 1> matchMixedSmallText(smallMixedText16);
	
	TBufC16<ARRAY_SIZE(largeErrMixedTextLast16) - 1> nonMatchMixedLargeTextLast(largeErrMixedTextLast16);
	TBufC16<ARRAY_SIZE(largeErrMixedTextFirst16) - 1> nonMatchMixedLargeTextFirst(largeErrMixedTextFirst16);
	
	TBufC16<ARRAY_SIZE(smallErrMixedTextLast16) - 1> nonMatchMixedSmallTextLast(smallErrMixedTextLast16);
	TBufC16<ARRAY_SIZE(smallErrMixedTextFirst16) - 1> nonMatchMixedSmallTextFirst(smallErrMixedTextFirst16);
		
	// Run performance tests...
	
	TheTest.Printf(_L("\n====== (8-bit) Ascii Performance tests ======"));
	
	// Matching (8-bit) Ascii
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nMatching Large Ascii Text\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiLargeText8, matchAsciiLargeText8, ETrue);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nMatching Small Ascii Text\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiSmallText8, matchAsciiSmallText8, ETrue);
	
	// Non Matching (8-bit) Ascii
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Ascii Text / Last Char Diff / Large Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiLargeText8, nonMatchAsciiLargeTextLast8, EFalse, -9);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Ascii Text / First Char Diff / Large Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiLargeText8, nonMatchAsciiLargeTextFirst8, EFalse, -23);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Ascii Text / Last Char Diff / Small Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiSmallText8, nonMatchAsciiSmallTextLast8, EFalse, -19);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Ascii Text / First Char Diff / Small Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiSmallText8, nonMatchAsciiSmallTextFirst8, EFalse, -23);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Ascii Text / Last Char Diff / Large Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiLargeText8, nonMatchAsciiSmallTextLast8, EFalse, -19);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Ascii Text / First Char Diff / Large Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiLargeText8, nonMatchAsciiSmallTextFirst8, EFalse, -23);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Ascii Text / Last Char Diff / Small Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiSmallText8, nonMatchAsciiLargeTextLast8, EFalse, -45);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Ascii Text / First Char Diff / Small Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiSmallText8, nonMatchAsciiLargeTextFirst8, EFalse, -23);
	
	// Mismatching length (8-bit) Ascii
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Ascii Text / Length Mismatch / Small Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiSmallText8, oriAsciiLargeText8, EFalse, -45);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Ascii Text / Length Mismatch / Large Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiLargeText8, oriAsciiSmallText8, EFalse, 45);	
	
	TheTest.Printf(_L("\n====== (16-bit) Ascii Performance tests ======"));
	
	// Matching (16-bit) Ascii
		
	TheTest.Printf(_L("\nComparing Performance Times For: \nMatching Large Ascii Text\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiLargeText, matchAsciiLargeText, ETrue);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nMatching Small Ascii Text\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiSmallText, matchAsciiSmallText, ETrue); 
	
	// Non Matching (16-bit) Ascii
		   
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Ascii Text / Last Char Diff / Large Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiLargeText, nonMatchAsciiLargeTextLast, EFalse, -9);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Ascii Text / First Char Diff / Large Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiLargeText, nonMatchAsciiLargeTextFirst, EFalse, -23);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Ascii Text / Last Char Diff / Small Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiSmallText, nonMatchAsciiSmallTextLast, EFalse, -19);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Ascii Text / First Char Diff / Small Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiSmallText, nonMatchAsciiSmallTextFirst, EFalse, -23);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Ascii Text / Last Char Diff / Large Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiLargeText, nonMatchAsciiSmallTextLast, EFalse, -19);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Ascii Text / First Char Diff / Large Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiLargeText, nonMatchAsciiSmallTextFirst, EFalse, -23);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Ascii Text / Last Char Diff / Small Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiSmallText, nonMatchAsciiLargeTextLast, EFalse, -1);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Ascii Text / First Char Diff / Small Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiSmallText, nonMatchAsciiLargeTextFirst, EFalse, -23);
	
	// Mismatching length (16-bit) Ascii
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Ascii Text / Length Mismatch / Small Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiSmallText, oriAsciiLargeText, EFalse, -1);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Ascii Text / Length Mismatch / Large Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriAsciiLargeText, oriAsciiSmallText, EFalse, 1);
	
	TheTest.Printf(_L("\n====== (16-bit) Unicode Performance tests ======"));
	
	// Matching (16-bit) Unicode
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nMatching Large Unicode Text\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriUnicodeLargeText, matchUnicodeLargeText, ETrue);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nMatching Small Unicode Text\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriUnicodeSmallText, matchUnicodeSmallText, ETrue);

	// Non Matching (16-bit) Unicode
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Unicode Text / Last Char Diff / Large Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriUnicodeLargeText, nonMatchUnicodeLargeTextLast, EFalse, -3);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Unicode Text / First Char Diff / Large Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriUnicodeLargeText, nonMatchUnicodeLargeTextFirst, EFalse, -3);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Unicode Text / Last Char Diff / Small Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriUnicodeSmallText, nonMatchUnicodeSmallTextLast, EFalse, -3);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Unicode Text / First Char Diff / Small Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriUnicodeSmallText, nonMatchUnicodeSmallTextFirst, EFalse, -3);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Unicode Text / Last Char Diff / Large Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriUnicodeLargeText, nonMatchUnicodeSmallTextLast, EFalse, -3);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Unicode Text / First Char Diff / Large Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriUnicodeLargeText, nonMatchUnicodeSmallTextFirst, EFalse, -3);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Unicode Text / Last Char Diff / Small Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriUnicodeSmallText, nonMatchUnicodeLargeTextLast, EFalse, -1);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Unicode Text / First Char Diff / Small Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriUnicodeSmallText, nonMatchUnicodeLargeTextFirst, EFalse, -3);

	// Mismatching length (16-bit) Unicode

	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Unicode Text / Length Mismatch / Small Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriUnicodeSmallText, oriUnicodeLargeText, EFalse, -1);

	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Unicode Text / Length Mismatch / Large Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriUnicodeLargeText, oriUnicodeSmallText, EFalse, 1);
	
	TheTest.Printf(_L("\n====== (16-bit) Mixed Performance tests ======"));
	
	// Matching (16-bit) Mixed
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nMatching Large Mixed Text\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriMixedLargeText, matchMixedLargeText, ETrue);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nMatching Small Mixed Text\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriMixedSmallText, matchMixedSmallText, ETrue);

	// Non Matching (16-bit) Mixed
	   
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Mixed Text / Last Char Diff / Large Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriMixedLargeText, nonMatchMixedLargeTextLast, EFalse, -655);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Mixed Text / First Char Diff / Large Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriMixedLargeText, nonMatchMixedLargeTextFirst, EFalse, -675);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Mixed Text / Last Char Diff / Small Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriMixedSmallText, nonMatchMixedSmallTextLast, EFalse, -655);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Mixed Text / First Char Diff / Small Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriMixedSmallText, nonMatchMixedSmallTextFirst, EFalse, -675);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Mixed Text / Last Char Diff / Large Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriMixedLargeText, nonMatchMixedSmallTextLast, EFalse, -655);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Mixed Text / First Char Diff / Large Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriMixedLargeText, nonMatchMixedSmallTextFirst, EFalse, -675);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Mixed Text / Last Char Diff / Small Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriMixedSmallText, nonMatchMixedLargeTextLast, EFalse, -1);
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Mixed Text / First Char Diff / Small Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriMixedSmallText, nonMatchMixedLargeTextFirst, EFalse, -675);
	
	// Mismatching length (16-bit) Mixed
	
	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Small Mixed Text / Length Mismatch / Small Vs Large\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriMixedSmallText, oriMixedLargeText, EFalse, -1);

	TheTest.Printf(_L("\nComparing Performance Times For: \nNON Matching Large Mixed Text / Length Mismatch / Large Vs Small\n"));
	RunComparison_PerformanceTest(numberOfTimes, oriMixedLargeText, oriMixedSmallText, EFalse, 1);
	}
	


TInt E32Main()
    {
	TheTest.Title();

	TheTest.Start(_L("Folding - performance tests"));

    ::FindF_PerformanceTest();
    ::MatchF_PerformanceTest();
    ::CompareF_PerformanceTest();
    ::PlainTextPerformanceTest();
	::CompareVsCompareF_PerformanceTest();

	TheTest.End();
	TheTest.Close();

	return KErrNone;
    }
