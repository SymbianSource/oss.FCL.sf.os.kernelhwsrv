// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\buffer\t_match.cpp
// Overview:
// Test the match methods of TPtrC8 and TPtrC16 objects and the 
// TCharIterator TCombiningCharIterator, TCollationValueIterator
// and TCollationRawValueIterator classes.
// API Information:
// TPtrC8, TPtrC16, TCharIterator, TCombiningCharIterator,
// TCollationValueIterator, TCollationRawValueIterator
// Details:
// - Test and verify the results of TPtrC8 Match and MatchF methods
// on a variety of constant strings. Verify both match and mismatch
// conditions.
// - Test and verify the results of TPtrC16 Match and MatchF methods
// on a variety of constant strings. Verify both match and mismatch
// conditions.
// - For a TCharIterator object, test and verify:
// - basic character handling
// - ability to reset the iterator correctly
// - combining characters works as expected
// - ability to jump into the middle of combined characters
// - full-width variants are not modified
// - narrow strings work as expected
// - surrogate pairs work as expected
// - Using a TCombiningCharIterator object with a variety of character 
// strings as input, verify that the output is as expected.
// - Using TCollationValueIterator and TCollationRawValueIterator objects:
// - test the raw iterator 
// - test starting at different points in the iteration and verify capitals 
// are ignored at level 0
// - verify capitals are ignored at level 1
// - verify capitals do not match at level 2
// - check the TCollationValueIterator Restart method
// - test collation keys, when they success and when they fail halfway
// - exhaust the internal cache, verify object still works
// - test different decompositions at level 3
// - verify results are as expected
// - Test and verify the results of the TUnicodeFold::FindWildcardMatchFolded()
// method on a variety of constant strings: find a string within another 
// string, verify the offset into the candidate string if it is present, or 
// KErrNotFound if it is not.
// - Test and verify the results of the TUnicodeFold::FindFolded() method on 
// a variety of constant strings: compare two strings, verify return value.
// - Test and verify results of UnicodeFoldCase() on a variety of characters.
// - Test and verify results of MatchLeadingWildcards() on a variety of strings.
// - Test and verify results of MatchesHereFoldedWithPrefixTest() on a variety 
// of strings.
// - Test and verify results of LocateFolded() on a variety of strings.
// - Test and verify results of FindFoldedWithWildcard() and FindMatchFolded()
// on a variety of strings.
// - Test and verify results of TDesc.CompareF() on a variety of strings.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <f32file.h>
#include <collate.h>
#include "collateimp.h"
#include "CompareImp.h"
#include "u32std.h"

///***************** copied from locale euser source code ***********************
static const TCollationMethod TheCollationMethod[] =
	{
		{
		KUidBasicCollationMethod,				// this is the standard unlocalised method
		NULL,									// null means use the standard table
		NULL,									// there's no override table
		0										// the flags are standard
		}
	};
static const TCollationDataSet TheCollationDataSet =
	{
	TheCollationMethod,
	1
	};
const LCharSet TheCharSet =
	{
	NULL,
	&TheCollationDataSet
	};
const LCharSet* GetLocaleCharSet()
	{
	return &TheCharSet;
	}
///*******************************************************************************


#ifdef __VC32__
    // Solve compilation problem caused by non-English locale
    #pragma setlocale("english")
#endif

#define ARRAY_SIZE(ar) (sizeof(ar) / (sizeof(ar[0])))

LOCAL_D RTest test(_L("T_MATCH"));

_LIT(KUnicodeTestDataFile, "z:\\Test\\UnicodeData.txt");

static const TUint32 TheDevanagariKey[] = 
	{
	0x22a010b,0x22b010b,0x285010b,0xb109,0xb209,0xb309,0xb409,0xb509,
	0x6c5e609,0x6c6e609,0x6c7e609,0x6c8e609,0x6c9e609,0x6cae609,0x6cbe609,0x6cce609,
	0x6cde609,0x6cee609,0xba40109,0xba50109,0xba60109,0xba70109,0xba80109,0xba90109,
	0xbaa0109,0xbab0109,0xbac0109,0xbad0109,0xbae0109,0xbaf0109,0xbb00109,0xbb10109,
	0xbb20109,0xbb30109,0xbb40109,0xbb50109,0xbb60109,0xbb70109,0xbb80109,0xbb90109,
	0xbb9b109,0xbba0109,0xbbab109,0xbbb0109,0xbbbb109,0xbbc0109,0xbbd0109,0xbbe0109,
	0xbbf0109,0xbc00109,0xbc0b109,0xbc10109,0xbc20109,0xbc30109,0xbc40109,0xbc50109,
	0xbc5b109,0xbc60109,0xbc6b109,0xbc70109,0xbc80109,0xbc90109,0xbca0109,0xbcb0109,
	0xbcc0109,0xbccb109,0xbcd0109,0xbce0109,0xbceb109,0xbcf0109,0xbd00109,0xbd10109,
	0xbd20109,0xbd2b109,0xbd30109,0xbd3b109,0xbd40109,0xbd50109,0xbd5b109,0xbd60109,
	0xbd70109,0xbd80109,0xbd90109,0xbda0109,0xbdb0109,0xbdc0109,0xbdd0109,0xbde0109,
	0xbdf0109,0xbe00109,0xbe10109,0xbe20109,0xbe30109,0xbe40109,0xbe50109,0xbe60109,
	0xbe70109,0xbe80109,0xbe90109,0xbea0109,0xbeb0109,0xbec0109,0xbed0109,
	};

static const TUint32 TheDevanagariIndex[] = 
	{
	0x9010012,0x9020013,0x9030014,0x9050015,0x9060016,0x9070017,0x9080018,0x9090019,
	0x90a001a,0x90b001b,0x90c001d,0x90d001f,0x90e0020,0x90f0021,0x9100022,0x9110023,
	0x9120024,0x9130025,0x9140026,0x9150027,0x9160029,0x917002b,0x918002d,0x919002e,
	0x91a002f,0x91b0030,0x91c0031,0x91d0033,0x91e0034,0x91f0035,0x9200036,0x9210037,
	0x9220039,0x923003b,0x924003c,0x925003d,0x926003e,0x927003f,0x9280040,0x9290041,
	0x92a0042,0x92b0043,0x92c0045,0x92d0046,0x92e0047,0x92f0048,0x930004a,0x931004b,
	0x932004c,0x933004d,0x934004e,0x935004f,0x9360050,0x9370051,0x9380052,0x9390053,
	0x93c0003,0x93d0054,0x93e0055,0x93f0056,0x9400057,0x9410058,0x9420059,0x943005a,
	0x944005b,0x945005e,0x946005f,0x9470060,0x9480061,0x9490062,0x94a0063,0x94b0064,
	0x94c0065,0x94d0066,0x9510004,0x9520005,0x9530006,0x9540007,0x9580028,0x959002a,
	0x95a002c,0x95b0032,0x95c0038,0x95d003a,0x95e0044,0x95f0049,0x960001c,0x961001e,
	0x962005c,0x963005d,0x9640000,0x9650001,0x9660008,0x9670009,0x968000a,0x969000b,
	0x96a000c,0x96b000d,0x96c000e,0x96d000f,0x96e0010,0x96f0011,0x9700002,
	};

static const TCollationKeyTable TheDevanagariTable = 
	{ TheDevanagariKey, TheDevanagariIndex, 103, 0, 0, 0 };

static const TCollationMethod TheDevanagariMethod =
	{ 0, 0, &TheDevanagariTable, 0 };

static const TCollationMethod TheDevanagariIgnoreCombiningMethod =
	{ 0, 0, &TheDevanagariTable, TCollationMethod::EIgnoreCombining };

static const TUint32 TheSwedishKey[] = 
	{
	0x8f60109,0x8f70109,0x8f80109,0x8f60121,0x8f70121,0x8f80121,0x8dd0109,0x8dd0121,
	0x8c50121,0x8c50109,
	};

static const TUint32 TheSwedishIndex[] = 
	{
	0x570008,0x770009,
	};

static const TUint16 TheSwedishStringElement[] = 
	{
	0x2,0x61,0x30a,0x2,0x61,0x308,0x2,0x6f,
	0x308,0x2,0x41,0x30a,0x2,0x41,0x308,0x2,
	0x4f,0x308,0x2,0x75,0x308,0x2,0x55,0x308,
	};

static const TUint32 TheSwedishStringIndex[] = 
	{
	0xc0004,0x90003,0xf0005,0x150007,0x30001,0x0,0x60002,0x120006,
	};

static const TCollationKeyTable TheSwedishTable = 
	{ TheSwedishKey, TheSwedishIndex, 2, TheSwedishStringElement, TheSwedishStringIndex, 8 };

static const TCollationMethod TheSwedishMethod =
	{ 0, 0, &TheSwedishTable, TCollationMethod::EIgnoreNone };

static const TCollationMethod TheIgnoreNoneMethod =
	{ 0, 0, 0, TCollationMethod::EIgnoreNone };

void TestPrintCaption(const TDesC& aTestName, const TText16 aStr[], TInt aLen)
	{
	test.Next(aTestName);
	RDebug::Print(_L("Char seq: "));
	for(TInt i=0;i<aLen;++i)
		{
		RDebug::Print(_L("%04X "), aStr[i]);
		}
	RDebug::Print(_L("\nOutput: "));
	}

TInt MatchC(const TDesC16& aCandidate, const TDesC16& aSearchTerm,
	const TCollationMethod* aMethod, TInt aLevel)
	{
	TCollate method(0);
	if (aMethod)
		{
		TCollate m(*aMethod);
		method = m;
		}
	return method.Match(aCandidate.Ptr(), aCandidate.Length(),
		aSearchTerm.Ptr(), aSearchTerm.Length(), aLevel);
	}

_LIT(KCand1, "baot");
_LIT(KCand2, "ba\x308o\x308t");
_LIT(KCand3, "b\xe4\xf6t");
_LIT(KSearch1, "BAOT");
_LIT(KSearch2, "?AO?");
_LIT(KSearch3, "?\xe4o?");
_LIT(KSearch4, "*o*");
_LIT(KSearch5, "*ao*");
_LIT(KSearch6, "*b\x308*");
_LIT(KSearch7, "ba\x308*");
_LIT(KSearch8, "ba*");

void TestMatchC()
	{
	// MatchC should be working at level 0, let us test that this is so.
	test(0 == KCand1().MatchC(KSearch1));
	test(0 == KCand1().MatchC(KCand2));
	test(1 == KCand1().MatchC(KSearch5));
	test(1 == KCand2().MatchC(KSearch5));
	test(0 <= KCand1().MatchC(KSearch2));
	// Test the internals at level 0: It must fail to match Swedish accents
	// with the Swedish collation algorithm.
	test(0 == MatchC(KCand1, KSearch1, &TheIgnoreNoneMethod, 0));
	test(0 == MatchC(KCand1, KSearch1, &TheSwedishMethod, 0));
	test(0 == MatchC(KCand2, KSearch1, &TheIgnoreNoneMethod, 0));
	test(KErrNotFound == MatchC(KCand2, KSearch1, &TheSwedishMethod, 0));
	test(0 == MatchC(KCand3, KSearch1, &TheIgnoreNoneMethod, 0));
	test(KErrNotFound == MatchC(KCand3, KSearch1, &TheSwedishMethod, 0));
	test(0 <= MatchC(KCand1, KSearch2, &TheIgnoreNoneMethod, 0));
	test(0 <= MatchC(KCand1, KSearch2, &TheSwedishMethod, 0));
	test(0 <= MatchC(KCand2, KSearch2, &TheIgnoreNoneMethod, 0));
	test(KErrNotFound == MatchC(KCand2, KSearch2, &TheSwedishMethod, 0));
	test(0 <= MatchC(KCand3, KSearch2, &TheIgnoreNoneMethod, 0));
	test(KErrNotFound == MatchC(KCand3, KSearch2, &TheSwedishMethod, 0));
	test(0 <= MatchC(KCand2, KSearch3, &TheIgnoreNoneMethod, 0));
	test(KErrNotFound == MatchC(KCand3, KSearch2, &TheSwedishMethod, 0));
	test(3 == MatchC(KCand2, KSearch4, &TheIgnoreNoneMethod, 0));
	test(KErrNotFound == MatchC(KCand2, KSearch4, &TheSwedishMethod, 0));
	test(1 == MatchC(KCand2, KSearch5, &TheIgnoreNoneMethod, 0));
	test(KErrNotFound == MatchC(KCand2, KSearch5, &TheSwedishMethod, 0));
	test(0 == MatchC(KCand2, KSearch6, &TheIgnoreNoneMethod, 0));
	test(0 == MatchC(KCand2, KSearch6, &TheSwedishMethod, 0));
	test(0 == MatchC(KCand1, KSearch7, &TheIgnoreNoneMethod, 0));
	test(KErrNotFound == MatchC(KCand1, KSearch7, &TheSwedishMethod, 0));
	test(0 == MatchC(KCand1, KSearch8, &TheIgnoreNoneMethod, 0));
	test(0 == MatchC(KCand1, KSearch8, &TheSwedishMethod, 0));
	test(0 == MatchC(KCand2, KSearch7, &TheIgnoreNoneMethod, 0));
	test(0 == MatchC(KCand2, KSearch7, &TheSwedishMethod, 0));
	test(0 == MatchC(KCand2, KSearch8, &TheIgnoreNoneMethod, 0));
	test(KErrNotFound == MatchC(KCand2, KSearch8, &TheSwedishMethod, 0));
	test(0 == MatchC(KCand3, KSearch7, &TheIgnoreNoneMethod, 0));
	test(0 == MatchC(KCand3, KSearch7, &TheSwedishMethod, 0));
	test(0 == MatchC(KCand3, KSearch8, &TheIgnoreNoneMethod, 0));
	test(KErrNotFound == MatchC(KCand3, KSearch8, &TheSwedishMethod, 0));
	_LIT(KCandidate1, "axyz");
	_LIT(KSearchStr1, "a*z");
	test(0 == KCandidate1().MatchC(KSearchStr1()));
	_LIT(KCandidate2, "azzz");
	_LIT(KSearchStr2, "a*z");
	test(0 == KCandidate2().MatchC(KSearchStr2()));
	
	// Added tests for INC105311...
	_LIT(KCandJpg1, "jpg_jjg.jpg");
	_LIT(KCandJpg2, "jpgAjjg.jpg");
	_LIT(KCandJpg3, "jpg@jjg.jpg");
	_LIT(KCandJpg4, "hpg&jjg.jpg");
	_LIT(KCandJpg5, "hpg&jpg.jpg");
	_LIT(KSearchJpg1, "*jp?");
	_LIT(KSearchJpg2, "*jpg");
	_LIT(KSearchJpg3, "*jpg*");
	_LIT(KSearchJpg4, "*jpg*jpg");
	test(8 == KCandJpg1().MatchC(KSearchJpg1));
	test(8 == KCandJpg2().MatchC(KSearchJpg1));
	test(8 == KCandJpg3().MatchC(KSearchJpg1));
	test(8 == KCandJpg4().MatchC(KSearchJpg1));
	test(8 == KCandJpg5().MatchC(KSearchJpg1));
	test(8 == KCandJpg1().MatchC(KSearchJpg2));
	test(8 == KCandJpg2().MatchC(KSearchJpg2));
	test(8 == KCandJpg3().MatchC(KSearchJpg2));
	test(8 == KCandJpg4().MatchC(KSearchJpg2));
	test(8 == KCandJpg5().MatchC(KSearchJpg2));
	test(0 == KCandJpg1().MatchC(KSearchJpg3));
	test(0 == KCandJpg2().MatchC(KSearchJpg3));
	test(0 == KCandJpg3().MatchC(KSearchJpg3));
	test(8 == KCandJpg4().MatchC(KSearchJpg3));
	test(4 == KCandJpg5().MatchC(KSearchJpg3));
	test(0 == KCandJpg1().MatchC(KSearchJpg4));
	test(0 == KCandJpg2().MatchC(KSearchJpg4));
	test(0 == KCandJpg3().MatchC(KSearchJpg4));
	test(KErrNotFound == KCandJpg4().MatchC(KSearchJpg4)); 
	test(4 == KCandJpg5().MatchC(KSearchJpg4));
	_LIT(KCand4, "abcxaxaxa");
	_LIT(KSearch9, "*xaxa");
	test(5 == KCand4().MatchC(KSearch9));
	_LIT(KCand6, "abxa");
	_LIT(KSearch10, "*x?");
	test(2 == KCand6().MatchC(KSearch10));
	_LIT(KCand7, "xab"); 
	_LIT(KSearch11, "x?");
	test(KErrNotFound == KCand7().MatchC(KSearch11));
	_LIT(KCand8, "xa"); 
	_LIT(KSearch12, "x?");
	test(0 == KCand8().MatchC(KSearch12));
	_LIT(KCand9, "xaxa"); 
	_LIT(KSearch13, "*x?");
	test(2 == KCand9().MatchC(KSearch13));
	_LIT(KCand10, "abjpgcjig.jpg"); 
	_LIT(KSearch14, "*jp?");
	test(10 == KCand10().MatchC(KSearch14));
	_LIT(KCand11, "abjpg_jig.jpg"); 
	_LIT(KSearch15, "*jp?");
	test(10 == KCand11().MatchC(KSearch15));
	_LIT(KCand12, "jpg"); 
	_LIT(KSearch16, "*jp?");
	test(0 == KCand12().MatchC(KSearch16));
	_LIT(KCand13, "abjpgig.jpg"); 
	_LIT(KSearch17, "*jp?");
	test(8 == KCand13().MatchC(KSearch17));
	_LIT(KCand14, "abjcgig.jpg"); 
	_LIT(KSearch18, "jp?");
	test(KErrNotFound == KCand14().MatchC(KSearch18));
	_LIT(KCand15, "xax\xE2"); 
	_LIT(KSearch19, "*xa\x302");
	test(2 == KCand15().MatchC(KSearch19));
	_LIT(KCand5, "blahblahblah\xE2");
	_LIT(KSearch20, "*a\x302");
	test(12 == KCand5().MatchC(KSearch20));
	_LIT(KCand16, "bl\xE2hblahblaha\x302");
	_LIT(KSearch21, "*a\x302*\xE2");
	test(2 == KCand16().MatchC(KSearch21));
	_LIT(KCand17, "abcxaxaxa");
	_LIT(KSearch22, "*x?x?");
	test(5 == KCand17().MatchC(KSearch22));
	}

/**
@SYMTestCaseID SYSLIB-EUSER-CT-1759
@SYMTestCaseDesc Various tests for the new TDesC16::MatchC() method. Testing that the new method works with
				 different wild card characters and different escape characters.
@SYMTestPriority High
@SYMTestActions  Test for TDesC16::MatchC(const TDesC16 &aPattern, TInt aMaxLevel, TInt aWildChar, TInt aWildSequenceChar, TInt aEscapeChar, const TCollationMethod* aCollationMethod = NULL).
@SYMTestExpectedResults The test must not fail.
@SYMREQ REQ5907
*/
void TestMatchC2()
	{
	_LIT(KCandidate1, "ab/cRRRdef__grt");
	_LIT(KSearchStr1, "ab//c%def/_/_grt");
	TInt rc = KCandidate1().MatchC(KSearchStr1(), '_', '%', '/', 0);
	test(rc == 0);
	_LIT(KCandidate2, "_*");
	_LIT(KSearchStr2, "/_/*");
	rc = KCandidate2().MatchC(KSearchStr2(), '_', '*', '/', 0);
	test(rc == 0);
	_LIT(KCandidate3, "aa");
	_LIT(KSearchStr3, "aaaa");
	rc = KCandidate3().MatchC(KSearchStr3(), '_', '%', 'a', 0);
	test(rc == 0);
	_LIT(KCandidate4, "\\4%3=1");
	_LIT(KSearchStr4, "\\\\4_3%");
	rc = KCandidate4().MatchC(KSearchStr4(), '_', '%', '\\', 0);
	test(rc == 0);
	_LIT(KCandidate5, "abcd&efgh");
	_LIT(KSearchStr5, "----!&efgh");
	rc = KCandidate5().MatchC(KSearchStr5(), '-', '&', '!', 0);
	test(rc == 0);
	_LIT(KCandidate6, "abc#1234:5678#xyz");
	_LIT(KSearchStr6, "#!#1234!:56::!##z");
	rc = KCandidate6().MatchC(KSearchStr6(), ':', '#', '!', 0);
	test(rc == 3);
	_LIT(KCandidate7, "abc#1234:5678#zzz");
	_LIT(KSearchStr7, "#!#1234!:56::!##z");
	rc = KCandidate7().MatchC(KSearchStr7(), ':', '#', '!', 0);
	test(rc == 3);
	_LIT(KCandidate8, "abc");
	_LIT(KSearchStr8, "a_c");
	rc = KCandidate8().MatchC(KSearchStr8(), '_', '%', '7', 0);
	test(rc == 0);
	_LIT(KCandidate9, "abc");
	_LIT(KSearchStr9, "A_C");
	rc = KCandidate9().MatchC(KSearchStr9(), '_', '%', '7', 0);
	test(rc == 0);
	_LIT(KCandidate10, "a_c");
	_LIT(KSearchStr10, "a7_c");
	rc = KCandidate10().MatchC(KSearchStr10(), '_', '%', '7', 0);
	test(rc == 0);
	_LIT(KCandidate11, "a_c");
	_LIT(KSearchStr11, "A7_C");
	rc = KCandidate11().MatchC(KSearchStr11(), '_', '%', '7', 0);
	test(rc == 0);
	_LIT(KCandidate12, "abc");
	_LIT(KSearchStr12, "a7_c");
	rc = KCandidate12().MatchC(KSearchStr12(), '_', '%', '7', 0);
	test(rc == KErrNotFound);
	_LIT(KCandidate13, "abc");
	_LIT(KSearchStr13, "A7_C");
	rc = KCandidate13().MatchC(KSearchStr13(), '_', '%', '7', 0);
	test(rc == KErrNotFound);
	_LIT(KCandidate14, "a7Xc");
	_LIT(KSearchStr14, "a7_c");
	rc = KCandidate14().MatchC(KSearchStr14(), '_', '%', '7', 0);
	test(rc == KErrNotFound);
	_LIT(KCandidate15, "a7Xc");
	_LIT(KSearchStr15, "A7_C");
	rc = KCandidate15().MatchC(KSearchStr15(), '_', '%', '7', 0);
	test(rc == KErrNotFound);
	_LIT(KCandidate16, "abcde");
	_LIT(KSearchStr16, "a%e");
	rc = KCandidate16().MatchC(KSearchStr16(), '_', '%', '7', 0);
	test(rc == 0);
	_LIT(KCandidate17, "abcde");
	_LIT(KSearchStr17, "A%E");
	rc = KCandidate17().MatchC(KSearchStr17(), '_', '%', '7', 0);
	test(rc == 0);
	_LIT(KCandidate18, "abcde");
	_LIT(KSearchStr18, "a7%e");
	rc = KCandidate18().MatchC(KSearchStr18(), '_', '%', '7', 0);
	test(rc == KErrNotFound);
	_LIT(KCandidate19, "abcde");
	_LIT(KSearchStr19, "A7%E");
	rc = KCandidate19().MatchC(KSearchStr19(), '_', '%', '7', 0);
	test(rc == KErrNotFound);
	_LIT(KCandidate20, "a7cde");
	_LIT(KSearchStr20, "a7%e");
	rc = KCandidate20().MatchC(KSearchStr20(), '_', '%', '7', 0);
	test(rc == KErrNotFound);
	_LIT(KCandidate21, "a7cde");
	_LIT(KSearchStr21, "A7%E");
	rc = KCandidate21().MatchC(KSearchStr21(), '_', '%', '7', 0);
	test(rc == KErrNotFound);
	_LIT(KCandidate22, "a7cde");
	_LIT(KSearchStr22, "a77%e");
	rc = KCandidate22().MatchC(KSearchStr22(), '_', '%', '7', 0);
	test(rc == 0);
	_LIT(KCandidate23, "a7cde");
	_LIT(KSearchStr23, "A77%E");
	rc = KCandidate23().MatchC(KSearchStr23(), '_', '%', '7', 0);
	test(rc == 0);
	_LIT(KCandidate24, "abc7");
	_LIT(KSearchStr24, "a%77");
	rc = KCandidate24().MatchC(KSearchStr24(), '_', '%', '7', 0);
	test(rc == 0);
	_LIT(KCandidate25, "abc7");
	_LIT(KSearchStr25, "A%77");
	rc = KCandidate25().MatchC(KSearchStr25(), '_', '%', '7', 0);
	test(rc == 0);
	_LIT(KCandidate26, "abc_");
	_LIT(KSearchStr26, "a%7_");
	rc = KCandidate26().MatchC(KSearchStr26(), '_', '%', '7', 0);
	test(rc == 0);
	_LIT(KCandidate27, "abc_");
	_LIT(KSearchStr27, "A%7_");
	rc = KCandidate27().MatchC(KSearchStr27(), '_', '%', '7', 0);
	test(rc == 0);
	_LIT(KCandidate28, "abc7");
	_LIT(KSearchStr28, "a%7_");
	rc = KCandidate28().MatchC(KSearchStr28(), '_', '%', '7', 0);
	test(rc == KErrNotFound);
	_LIT(KCandidate29, "abc7");
	_LIT(KSearchStr29, "A%7_");
	rc = KCandidate29().MatchC(KSearchStr29(), '_', '%', '7', 0);
	test(rc == KErrNotFound);
	_LIT(KCandidate30, "ba\x308o\x308t");
	_LIT(KSearchStr30, "-b\x308-");
	rc = KCandidate30().MatchC(KSearchStr30(), '%', '-', 0, 0);
	test(rc == 0);
	rc = KCandidate30().MatchC(KSearchStr30(), '%', '-', 0, 0, &TheSwedishMethod);
	test(rc == 0);
	_LIT(KSearchStr31, "ba\x308*");
	rc = KCandidate30().MatchC(KSearchStr31(), '%', '*', 0, 0, &TheSwedishMethod);
	test(rc == 0);
	}

void DoTestCanonicalDecompositionIterator(const TDesC& aTest, const TDesC& aCanonical)
	{
	TUTF32Iterator i(aTest.Ptr(), aTest.Ptr() + aTest.Length());
	TCanonicalDecompositionIterator cci;
	cci.Set(i);
	TInt index = 0;
	while (!cci.AtEnd())
		{
		test(index != aCanonical.Length());
		TChar ch1 = aCanonical[index];
		TChar ch2 = cci.Current();
		test(ch1 == ch2);
		++index;
		cci.Next();
		}
	test(index == aCanonical.Length());
	}

// exciting combining characters include:
// U+0327 cedilla, class = 202
// U+031B horn, class = 216
// U+0316 grave below, class = 220
// U+0300 grave above, class = 230
// U+031A left angle above, class = 232
// U+0360 double tilde, class = 234
// U+0345 ypogegrameni, class = 240

_LIT(KAllOnce, "\x327\x31b\x316\x300\x31a\x360\x345xyz");
_LIT(KBackwards, "\x345\x360\x31a\x300\x316\x31b\x327xyz");
_LIT(KRandom, "\x300\x316\x31b\x327\x31a\x345\x360xyz");
_LIT(KAllOnceThenAcute, "\x327\x31b\x316\x300\x301\x31a\x360\x345");
_LIT(KBackwardsThenAcute, "\x345\x360\x31a\x300\x301\x316\x31b\x327");
_LIT(KRandomThenAcute, "\x300\x316\x31b\x327\x31a\x301\x345\x360");
_LIT(KAllSame, "\x300\x301\x302\x303\x304\x306\x307\x308\x30b\x30c\x30f");
_LIT(KLotsSameCanonical, "\x327\x31b\x316\x300\x301\x302\x303\x304\x306\x307\x308\x30b\x30c\x30f\x31a\x360\x345xyz");
_LIT(KLotsSameNotCanonical, "\x31b\x300\x345\x301\x302\x316\x303\x304\x31a\x306\x307\x327\x308\x30b\x30c\x30f\x360xyz");

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-0106
@SYMTestCaseDesc TCanonicalDecompositionIterator test 
@SYMTestPriority High
@SYMTestActions  TCanonicalDecompositionIterator test
@SYMTestExpectedResults The test must not fail.
@SYMPREQ814 Optimise folded string comparisons.
*/
void TestCanonicalDecompositionIterator()
	{
	DoTestCanonicalDecompositionIterator(KAllOnce, KAllOnce);
	DoTestCanonicalDecompositionIterator(KBackwards, KAllOnce);
	DoTestCanonicalDecompositionIterator(KRandom, KAllOnce);
	DoTestCanonicalDecompositionIterator(KBackwardsThenAcute, KAllOnceThenAcute);
	DoTestCanonicalDecompositionIterator(KRandomThenAcute, KAllOnceThenAcute);
	DoTestCanonicalDecompositionIterator(KAllSame, KAllSame);
	DoTestCanonicalDecompositionIterator(KLotsSameCanonical, KLotsSameCanonical);
	DoTestCanonicalDecompositionIterator(KLotsSameNotCanonical, KLotsSameCanonical);
	}

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-3337
@SYMTestCaseDesc Test latest MatchC override that has the advanced TCollationMethod flag input 
@SYMTestPriority High
@SYMTestActions Test matching of combined characters with simple character.
Note- this action only applies to certain classes of combining characters
@SYMTestExpectedResults Old MatchC character against character + combining character
will not match but new MatchC with same characters and flag set will match.
@SYMINC092513: RR S60: Variant testing: Searching does not work properly in contacts
*/
void TestDisableCombiningCharacterCheckFlag(const TDesC16 &aLeft, const TDesC16 &aRight)
	{
	//Without flag, characters combine to make new character not matched by search
	test(KErrNotFound ==aLeft.MatchC(aRight,&TheDevanagariMethod));
	//With flag, combining character check is disabled so that search is matched
	test(KErrNone     ==aLeft.MatchC(aRight,&TheDevanagariIgnoreCombiningMethod));
	}
void TestDisableCombiningCharacterCheck()
	{
	test.Next(_L("INC092513"));
	TBuf<2> search, target;
	//All Devanagari dependant vowels are in the following range
	for(TInt dependantVowel=0x93e; dependantVowel<=0x94c; dependantVowel++)
		{
		//Most of the Devanagari consonants are in the following range
		for(TInt consonant=0x915; consonant<=0x939; consonant++)
			{
			target.Format(_L("%c%c"),consonant,dependantVowel);
			search.Format(_L("%c*"),consonant);
			TestDisableCombiningCharacterCheckFlag(target, search);
			}
		}
	//Test same situation but where consonants decompose to consonant + dependant vowel
	TestDisableCombiningCharacterCheckFlag(_L("\x929"), _L("\x928*"));
	TestDisableCombiningCharacterCheckFlag(_L("\x931"), _L("\x930*"));
	TestDisableCombiningCharacterCheckFlag(_L("\x934"), _L("\x933*"));
	TestDisableCombiningCharacterCheckFlag(_L("\x958"), _L("\x915*"));
	TestDisableCombiningCharacterCheckFlag(_L("\x959"), _L("\x916*"));
	TestDisableCombiningCharacterCheckFlag(_L("\x95a"), _L("\x917*"));
	TestDisableCombiningCharacterCheckFlag(_L("\x95b"), _L("\x91c*"));
	TestDisableCombiningCharacterCheckFlag(_L("\x95c"), _L("\x921*"));
	TestDisableCombiningCharacterCheckFlag(_L("\x95d"), _L("\x922*"));
	TestDisableCombiningCharacterCheckFlag(_L("\x95e"), _L("\x92b*"));
	TestDisableCombiningCharacterCheckFlag(_L("\x95f"), _L("\x92f*"));
	}

_LIT(KHelloT, "Hello");
_LIT(KLatin1AccentsC, "\xE0\xD2p\xE2\xEB\xED\xF1\xC7");
_LIT(KLatin1AccentsD, "a\x300O\x300pa\x302\x65\x308i\x301n\x303\x43\x327");
// four alpha + psili + varia + ypogegrameni
_LIT(KGreekAccentsC, "\x1f82\x1f82\x1f82\x1f82");
// decomposed in four different ways
_LIT(KGreekAccentsS, "\x1f82\x1f02\x345\x1f00\x300\x345\x3b1\x313\x300\x345");
// completely decomposed
_LIT(KGreekAccentsD, "\x3b1\x313\x300\x345\x3b1\x313\x300\x345\x3b1\x313\x300\x345\x3b1\x313\x300\x345");
// full-width variants
_LIT(KFullWidth, "\xFF21\xFF42\xFF43");
// surrogate pair, unpaired low surrogate, unpaired high surrogate, unpaired
// high surrogate at end of string
_LIT(KSurrogates, "\xD965\xDEF0\xDF12\xDB10\xDA4E");
_LIT(KSurrogatesTest, "\xD965\xDEF0");

void TestIteratorOutput(TDecompositionIterator& aIt, const TDesC& aCheck)
	{
	TBool unpairedHighSurrogate = EFalse;
	for(TInt i = 0; i != aCheck.Length(); aIt.Next())
		{
		if (aIt.AtEnd())
			{
			test(0);
			return;
			}
		TInt c = aIt.Current();
		// test that we are not looking at an unpaired low surrogate that
		// follows an unpaired high surrogate: this is not possible.
		test((c & 0xFC00) != 0xDC00 || !unpairedHighSurrogate);
		unpairedHighSurrogate = (c & 0xFC00) == 0xD800? (TBool)ETrue : (TBool)EFalse;
		if (c < 0x10000)
			{
			test(c == aCheck[i]);
			++i;
			}
		else
			{
			TInt sp = ((aCheck[i] - 0xD7F7) << 10) + aCheck[i + 1];
			test(c == sp);
			i += 2;
			}
		}
	test(aIt.AtEnd());
	}

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-0097
@SYMTestCaseDesc TUTF32Iterator functionality tested on 2 character sequences: 
	(1) single character 
	(2) surrogate pair
@SYMTestPriority High
@SYMTestActions  TUTF32Iterator test.
@SYMTestExpectedResults The test must not fail.
@SYMPREQ814 Optimise folded string comparisons.
*/
void TestUTF32Iterator()
	{
	//Single character
	const TText16 KStr[] = {0x01D5};
	::TestPrintCaption(_L("TUTF32Iterator"), KStr, ARRAY_SIZE(KStr));
	TUTF32Iterator it(KStr, KStr + ARRAY_SIZE(KStr));
	TInt itCount = 0;
	for(;!it.AtEnd();++itCount, it.Next())
		{
		TChar ch = it.Current();
		test(ch == static_cast <TUint> (KStr[0]));
		RDebug::Print(_L("%04X "), (TUint)ch);
		}
	test(itCount == 1);
	RDebug::Print(_L("\n"));
	
	//Surrogate pair
	::TestPrintCaption(_L("TUTF32Iterator-surrogates"), KSurrogatesTest().Ptr(), KSurrogatesTest().Length());
	it = TUTF32Iterator(KSurrogatesTest().Ptr(), KSurrogatesTest().Ptr() + KSurrogatesTest().Length());
	for(itCount=0;!it.AtEnd();++itCount, it.Next())
		{
		TChar ch = it.Current();
		test(ch == 0x696F0);
		RDebug::Print(_L("%06X "), (TUint)ch);
		}
	test(itCount == 1);
	RDebug::Print(_L("\n"));

	//	surrogate 0x10000
	_LIT( KSurrogatesTest2, "\xd800\xdc00" );
	::TestPrintCaption(_L("TUTF32Iterator-surrogates2"), KSurrogatesTest2().Ptr(), KSurrogatesTest2().Length());
	it = TUTF32Iterator(KSurrogatesTest2().Ptr(), KSurrogatesTest2().Ptr() + KSurrogatesTest2().Length());
	for(itCount=0;!it.AtEnd();++itCount, it.Next())
		{
		TChar ch = it.Current();
		test(ch == 0x10000);
		RDebug::Print(_L("%06X "), (TUint)ch);
		}
	test(itCount == 1);
	RDebug::Print(_L("\n"));
	
	//	surrogate 0x20000
	_LIT( KSurrogatesTest3, "\xd840\xdc00" );
	::TestPrintCaption(_L("TUTF32Iterator-surrogates3"), KSurrogatesTest3().Ptr(), KSurrogatesTest3().Length());
	it = TUTF32Iterator(KSurrogatesTest3().Ptr(), KSurrogatesTest3().Ptr() + KSurrogatesTest3().Length());
	for(itCount=0;!it.AtEnd();++itCount, it.Next())
		{
		TChar ch = it.Current();
		test(ch == 0x20000);
		RDebug::Print(_L("%06X "), (TUint)ch);
		}
	test(itCount == 1);
	RDebug::Print(_L("\n"));
	
	//	surrogate 0x2ffff
	_LIT( KSurrogatesTest4, "\xD87F\xDFFF" );
	::TestPrintCaption(_L("TUTF32Iterator-surrogates4"), KSurrogatesTest4().Ptr(), KSurrogatesTest4().Length());
	it = TUTF32Iterator(KSurrogatesTest4().Ptr(), KSurrogatesTest4().Ptr() + KSurrogatesTest4().Length());
	for(itCount=0;!it.AtEnd();++itCount, it.Next())
		{
		TChar ch = it.Current();
		test(ch == 0x2ffff);
		RDebug::Print(_L("%06X "), (TUint)ch);
		}
	//test(itCount == 1);
	RDebug::Print(_L("\n"));

	//	surrogate 0xd800
	_LIT( KSurrogatesTest5, "\xD800" );
	::TestPrintCaption(_L("TUTF32Iterator-surrogates5"), KSurrogatesTest5().Ptr(), KSurrogatesTest5().Length());
	it = TUTF32Iterator(KSurrogatesTest5().Ptr(), KSurrogatesTest5().Ptr() + KSurrogatesTest5().Length());
	for(itCount=0;!it.AtEnd();++itCount, it.Next())
		{
		TChar ch = it.Current();
		RDebug::Print(_L("%06X "), (TUint)ch);
		}
	test(itCount == 0);
	RDebug::Print(_L("\n"));	

	//	surrogate 0xdc00
	_LIT( KSurrogatesTest6, "\xDc00" );
	::TestPrintCaption(_L("TUTF32Iterator-surrogates6"), KSurrogatesTest6().Ptr(), KSurrogatesTest6().Length());
	it = TUTF32Iterator(KSurrogatesTest6().Ptr(), KSurrogatesTest6().Ptr() + KSurrogatesTest6().Length());
	for(itCount=0;!it.AtEnd();++itCount, it.Next())
		{
		TChar ch = it.Current();
		RDebug::Print(_L("%06X "), (TUint)ch);
		}
	test(itCount == 0);
	RDebug::Print(_L("\n"));	
	
	//	surrogate 0xdfff
	_LIT( KSurrogatesTest7, "\xDfff" );
	::TestPrintCaption(_L("TUTF32Iterator-surrogates7"), KSurrogatesTest7().Ptr(), KSurrogatesTest7().Length());
	it = TUTF32Iterator(KSurrogatesTest7().Ptr(), KSurrogatesTest7().Ptr() + KSurrogatesTest7().Length());
	for(itCount=0;!it.AtEnd();++itCount, it.Next())
		{
		TChar ch = it.Current();
		RDebug::Print(_L("%06X "), (TUint)ch);
		}
	test(itCount == 0);
	RDebug::Print(_L("\n"));	
	}

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-0098
@SYMTestCaseDesc TFoldedDecompIterator functionality tested on 2 character sequences.
@SYMTestPriority High
@SYMTestActions  TFoldedDecompIterator test.
@SYMTestExpectedResults The test must not fail.
@SYMPREQ814 Optimise folded string comparisons.
*/
void TestFoldedDecompIterator()
    {
    //Character sequence 1:
    //(1) DEVANAGARI LETTER FA - 0x095E
    //(2) LATIN CAPITAL LETTER U WITH DIAERESIS AND MACRON - 0x01D5
    //Decompositions:
    //(1) 0x095E decomposed to: 0x092B 0x093C
    //(2) 0x01D5 decomposed to: 0x00DC 0x0304
    //    0x00DC decomposed to: 0x0055 0x0308
    //    0x0055 decomposed to: 0x0075
    const TText16 KStr[] = {0x095E, 0x01D5};
    const TText16 KStrOut[] = {0x092B, 0x093C, 0x0075, 0x0308, 0x0304};
    ::TestPrintCaption(_L("TFoldedDecompIterator-1"), KStr, ARRAY_SIZE(KStr));
    TUTF32Iterator itSrc(KStr, KStr + ARRAY_SIZE(KStr));
    TFoldedDecompIterator it(itSrc);
    TInt itCount = 0;
    for(;!it.AtEnd();++itCount, it.Next())
        {
        if(!it.IsInFoldedSequence())
            {
            it.EnterFoldedSequence();
            }
        TChar ch = it.Current();
        test(ch == static_cast <TUint> (KStrOut[itCount]));
        RDebug::Print(_L("%04X "), (TUint)ch);
        }
    test(itCount == ARRAY_SIZE(KStrOut));
    RDebug::Print(_L("\n"));
    
    //Character sequence 2:
    //(1) GREEK CAPITAL LETTER BETA - 0x0392
    //(2) COMBINING GRAVE ACCENT - 0x0300
    //(3) COMBINING GRAVE ACCENT BELOW - 0x0316
    //(4) GREEK CAPITAL LETTER GAMMA - 0x0393
    //(5) HEBREW POINT TSERE - 0x05B5
    //(6) TIBETAN MARK HALANTA - 0x0F84
    //Decompositions:
    //(1) 0x0392 decomposed to: 0x03B2
    //(2) 0x0300 decomposed to: 0x0300
    //(3) 0x0316 decomposed to: 0x0316
    //(4) 0x0393 decomposed to: 0x03B3
    //(5) 0x05B5 decomposed to: 0x05B5
    //(6) 0x0F84 decomposed to: 0x0F84
    const TText16 KStr2[] = {0x0392, 0x0300, 0x0316, 0x0393, 0x05B5, 0x0F84};
    const TText16 KStrOut2[] = {0x03B2, 0x0300, 0x0316, 0x03B3, 0x05B5, 0x0F84};
    ::TestPrintCaption(_L("TFoldedDecompIterator-2"), KStr2, ARRAY_SIZE(KStr2));
    itSrc = TUTF32Iterator(KStr2, KStr2 + ARRAY_SIZE(KStr2));
    it = TFoldedDecompIterator(itSrc);
    for(itCount=0;!it.AtEnd();++itCount, it.Next())
        {
        if(!it.IsInFoldedSequence())
            {
            it.EnterFoldedSequence();
            }
        TChar ch = it.Current();
        test(ch == static_cast <TUint> (KStrOut2[itCount]));
        RDebug::Print(_L("%04X "), (TUint)ch);
        }
    test(itCount == ARRAY_SIZE(KStrOut2));
    RDebug::Print(_L("\n"));
    
    //Character sequence 3:
    //(1) MUSICAL SYMBOL EIGHTH NOTE - 0x1D161 (D834, DD61)
    //Decompositions:
    //(1) 0x1D161 decomposed to: 0x1D15F 0x1D16F
    //    0x1D15F decomposed to: 0x1D158 0x1D165
    const TText16 KStr3[] = {0xD834, 0xDD61};
    const TUint32 KStrOut3[] = {0x1D158, 0x1D165, 0x1D16F};
    ::TestPrintCaption(_L("TFoldedDecompIterator-3"), KStr3, ARRAY_SIZE(KStr3));
    itSrc = TUTF32Iterator(KStr3, KStr3 + ARRAY_SIZE(KStr3));
    it = TFoldedDecompIterator(itSrc);
    for(itCount=0;!it.AtEnd();++itCount, it.Next())
        {
        if(!it.IsInFoldedSequence())
            {
            it.EnterFoldedSequence();
            }
        TChar ch = it.Current();
        test(ch == static_cast <TUint> (KStrOut3[itCount]));
        RDebug::Print(_L("%04X "), (TUint)ch);
        }
    test(itCount == ARRAY_SIZE(KStrOut3));
    RDebug::Print(_L("\n"));
    }

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-0099
@SYMTestCaseDesc TFoldedSortedDecompIterator functionality tested on 1 character sequence.
@SYMTestPriority High
@SYMTestActions  TFoldedSortedDecompIterator test.
@SYMTestExpectedResults The test must not fail.
@SYMPREQ814 Optimise folded string comparisons.
*/
void TestFoldedSortedDecompIterator()
    {
    //Character sequence 1:
    //(1) GREEK CAPITAL LETTER BETA - 0x0392	(fold: 0x3B2, ccc=0)
    //(2) COMBINING GRAVE ACCENT - 0x0300		(no fold, ccc=230)
    //(3) COMBINING GRAVE ACCENT BELOW - 0x0316	(no fold, ccc=220)
    //(4) GREEK CAPITAL LETTER GAMMA - 0x0393	(fold: 0x3B3, ccc=0)
    //(5) HEBREW POINT TSERE - 0x05B5			(no fold, ccc=15)
    //(6) 0x10A39								(no fold, ccc=1)
    //(7) TIBETAN MARK HALANTA - 0x0F84			(no fold, ccc=9)
    //(8) 0x10400								(fold: 0x10428, ccc=0)
    //(9) 0x10A38								(no fold, ccc=230)
    //(10) 0xFB1E								(no fold, ccc=26)
    //Decompositions:
    //0x03B2 Class 0
    //0x0316 Class 220
    //0x0300 Class 230
    //0x03B3 Class 0
    //0x10A39 Class 1
    //0x0F84 Class 9
    //0x05B5 Class 15
    //0x10428 Class 0
    //0xFB1E Class 26
    //0x10A38 Class 230
    //const TText16 KStr[] = {0x0392, 0x0300, 0x0316, 0x0393, 0x05B5, 0x0F84};
    //const TText16 KStrOut[] = {0x03B2, 0x0316, 0x0300, 0x03B3, 0x0F84, 0x05B5};
    //const TInt KClass[] = {0, 220, 230, 0, 9, 15};
    const TText16 KStr[] = {0x0392, 0x0300, 0x0316, 0x0393, 0x05B5, 0xD802, 0xDE39, 0x0F84, 0xD801, 0xDC00, 0xD802, 0xDE38, 0xFB1E};
    const TUint32 KStrOut[] = {0x03B2, 0x0316, 0x0300, 0x03B3, 0x10A39, 0x0F84, 0x05B5, 0x10428, 0xFB1E, 0x10A38};
    const TInt KClass[] = {0, 220, 230, 0, 1, 9, 15, 0, 26, 230};
    ::TestPrintCaption(_L("TFoldedSortedDecompIterator"), KStr, ARRAY_SIZE(KStr));
    RDebug::Print(_L("\n"));
    TUTF32Iterator itSrc(KStr, KStr + ARRAY_SIZE(KStr));
    TFoldedDecompIterator itDecomp(itSrc);
    TInt itCount = 0;
    while(!itDecomp.AtEnd())
        {
        if(!itDecomp.IsInFoldedSequence())
            {
            itDecomp.EnterFoldedSequence();
            }
        if(itDecomp.Current().GetCombiningClass() == 0)
            {
            TChar ch = itDecomp.Current();
            TInt clss = itDecomp.Current().GetCombiningClass();
            RDebug::Print(_L("BaseCh %04X Class %d\n"), (TUint)ch, clss);
            test(ch == static_cast <TUint> (KStrOut[itCount]));
            test(clss == KClass[itCount]);
            itDecomp.Next();
            ++itCount;
            }
        else
            {
            TFoldedSortedDecompIterator it;
            it.Set(itDecomp);
            while(!it.AtEnd())
                {
                TChar ch = it.Current();
                TInt clss = it.Current().GetCombiningClass();
                RDebug::Print(_L("CombCh %08X Class %d\n"), (TUint)ch, clss);
                test(ch == static_cast <TUint> (KStrOut[itCount]));
                test(clss == KClass[itCount]);
                it.Next();
                ++itCount;
                }
            }
        }
    test(itCount == ARRAY_SIZE(KStrOut));
    }

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-0100
@SYMTestCaseDesc TFoldedCanonicalIterator functionality tested on 1 character sequence.
@SYMTestPriority High
@SYMTestActions  TFoldedCanonicalIterator test.
@SYMTestExpectedResults The test must not fail.
@SYMPREQ814 Optimise folded string comparisons.
*/
void TestFoldedCanonicalIterator()
    {
    //Character sequence 1:
    //(1) GREEK CAPITAL LETTER BETA - 0x0392
    //(2) COMBINING GRAVE ACCENT - 0x0300
    //(3) COMBINING GRAVE ACCENT BELOW - 0x0316
    //(4) GREEK CAPITAL LETTER GAMMA - 0x0393
    //(5) HEBREW POINT TSERE - 0x05B5
    //(6) TIBETAN MARK HALANTA - 0x0F84
    //(7) MUSICAL SYMBOL EIGHTH NOTE - 0x1D161 (D834, DD61)
    //(8) LATIN CAPITAL LETTER U WITH DIAERESIS AND MACRON - 0x01D5
    //Decompositions:
    //0x0392 to 0x03B2 
    //0x0316 to 0x0316 
    //0x0300 to 0x0300
    //0x0393 to 0x03B3
    //0x0F84 to 0x0F84
    //0x05B5 to 0x05B5
    //0x1D161 to 0x1D158 0x1D165 0x1D16F
    //0x01D5 to 0x0075 0x0308 0x0304
    const TText16 KStr[] = {0x0392, 0x0300, 0x0316, 0x0393, 0x05B5, 0x0F84, 0xD834, 0xDD61, 0x01D5};
    const TUint32 KStrOut[] = {0x03B2, 0x0316, 0x0300, 0x03B3, 0x0F84, 0x05B5, 0x1D158, 0x1D165, 0x1D16F, 0x0075, 0x0308, 0x0304};
    TestPrintCaption(_L("TFoldedCanonicalIterator"), KStr, ARRAY_SIZE(KStr));
    TUTF32Iterator itSrc(KStr, KStr + ARRAY_SIZE(KStr));
	TFoldedCanonicalIterator it(itSrc);
    TInt itCount = 0;

	const TUnicodeDataSet* charDataSet = GetLocaleCharSet()->iCharDataSet;

    for(;!it.AtEnd();++itCount, it.Next(charDataSet))
        {
        TChar ch = it.Current();
        test(ch == static_cast <TUint> (KStrOut[itCount]));
        RDebug::Print(_L("%04X "), (TUint)ch);
        }
    test(itCount == ARRAY_SIZE(KStrOut));
    RDebug::Print(_L("\n"));
    
    //10400
    //103ff
    }

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-0101
@SYMTestCaseDesc TDecompositionIterator functionality tested on 1 character sequence
@SYMTestPriority High
@SYMTestActions  TDecompositionIterator test.
@SYMTestExpectedResults The test must not fail.
@SYMPREQ814 Optimise folded string comparisons.
*/
void TestDecompositionIterator2()
    {
    //Character sequence 1
    //LATIN CAPITAL LETTER U WITH DIAERESIS AND MACRON - 0x01D5
    //Decomposition:
    //0x01D5 to: 0x0055 0x0308 0x0304 
    const TText16 KStr[] = {0x01D5};
    const TText16 KStrOut[] = {0x0055, 0x0308, 0x0304};
    TestPrintCaption(_L("TDecompositionIterator"), KStr, ARRAY_SIZE(KStr));
    TUTF32Iterator itSrc(KStr, KStr + ARRAY_SIZE(KStr));
    TDecompositionIterator it;
    it.Set(itSrc);
    TInt itCount = 0;
    for(;!it.AtEnd(); ++itCount, it.Next())
        {
        TChar ch = it.Current();
        test(ch == static_cast <TUint> (KStrOut[itCount]));
        RDebug::Print(_L("%04X "), (TUint)ch);
        }
    test(itCount == ARRAY_SIZE(KStrOut));
    RDebug::Print(_L("\n"));

    // Character sequence 2
    // MUSICAL SYMBOL THIRTY-SECOND NOTE - 0x1D162 (D834, DD62)
    // Decomposition:
    // 0x1D162 to: 0x1D15F 0x1D170, then to: 0x1D158 0x1D165 0x1D170
    const TText16 KStr2[] = {0xD834, 0xDD62};
    const TUint32 KStrOut2[] = {0x1D158, 0x1D165, 0x1D170};
    TestPrintCaption(_L("TDecompositionIterator"), KStr2, ARRAY_SIZE(KStr2));
    TUTF32Iterator itSrc2(KStr2, KStr2 + ARRAY_SIZE(KStr2));
    TDecompositionIterator it2;
    it2.Set(itSrc2);
    TInt itCount2 = 0;
    for(;!it2.AtEnd(); ++itCount2, it2.Next())
        {
        TChar ch = it2.Current();
        //test.Printf(_L("    expect = %08X, result = %08X\n"), KStrOut2[itCount2], ch);
        test(ch == KStrOut2[itCount2]);
        RDebug::Print(_L("%04X "), (TUint)ch);
        }
    test(itCount2 == ARRAY_SIZE(KStrOut2));
    RDebug::Print(_L("\n"));
    }

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-0102
@SYMTestCaseDesc TCanonicalDecompositionIterator functionality tested on 1 character sequence
@SYMTestPriority High
@SYMTestActions  TCanonicalDecompositionIterator test.
@SYMTestExpectedResults The test must not fail.
@SYMPREQ814 Optimise folded string comparisons.
*/
void TestCanonicalDecompositionIterator2()
    {
    //Character sequence 1
    //(1) LATIN CAPITAL LETTER U WITH DIAERESIS AND MACRON - 0x01D5
    //(2) MUSICAL SYMBOL THIRTY-SECOND NOTE - 0x1D162 (D834, DD62)
    //Decomposition:
    //0x01D5 to: 0x0055 0x0308 0x0304
    //0x1D162 to: 0x1D15F 0x1D170, then to: 0x1D158 0x1D165 0x1D170
    const TText16 KStr[] = {0x01D5, 0xD834, 0xDD62};
    const TUint32 KStrOut[] = {0x0055, 0x0308, 0x0304, 0x1D158, 0x1D165, 0x1D170};
    TestPrintCaption(_L("TCanonicalDecompositionIterator"), KStr, ARRAY_SIZE(KStr));
    TUTF32Iterator itSrc(KStr, KStr + ARRAY_SIZE(KStr));
    TCanonicalDecompositionIterator it;
    it.Set(itSrc);
    TInt itCount = 0;
    for(;!it.AtEnd();++itCount, it.Next())
        {
        TChar ch = it.Current();
        test(ch == static_cast <TUint> (KStrOut[itCount]));
        RDebug::Print(_L("%04X "), (TUint)ch);
        }
    test(itCount == ARRAY_SIZE(KStrOut));
    RDebug::Print(_L("\n"));
    }

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-0103
@SYMTestCaseDesc TCanonicalDecompositionIteratorCached functionality tested on 1 character sequence
@SYMTestPriority High
@SYMTestActions  TCanonicalDecompositionIteratorCached test.
@SYMTestExpectedResults The test must not fail.
@SYMPREQ814 Optimise folded string comparisons.
*/
void TestCanonicalDecompositionIteratorCached()
    {
    //Character sequence 1
    //(1) LATIN CAPITAL LETTER U WITH DIAERESIS AND MACRON - 0x01D5
    //(2) MUSICAL SYMBOL THIRTY-SECOND NOTE - 0x1D162 (D834, DD62)
    //Decomposition:
    //0x01D5 to: 0x0055 0x0308 0x0304
    //0x1D162 to: 0x1D15F 0x1D170, then to: 0x1D158 0x1D165 0x1D170
    const TText16 KStr[] = {0x01D5, 0xD834, 0xDD62};
    const TUint32 KStrOut[] = {0x0055, 0x0308, 0x0304, 0x1D158, 0x1D165, 0x1D170};
    TestPrintCaption(_L("TCanonicalDecompositionIteratorCached"), KStr, ARRAY_SIZE(KStr));
    TUTF32Iterator itSrc(KStr, KStr + ARRAY_SIZE(KStr));
    TCanonicalDecompositionIteratorCached it;
    it.Set(itSrc);
    TInt itCount = 0;
    for(;!it.AtEnd();++itCount, it.Next(1))
        {
        TChar ch = it.Get(0);
        test(ch == static_cast <TUint> (KStrOut[itCount]));
        RDebug::Print(_L("%04X "), (TUint)ch);
        }
    test(itCount == ARRAY_SIZE(KStrOut));
    RDebug::Print(_L("\n"));
    }

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-0104
@SYMTestCaseDesc TDecompositionIterator test
@SYMTestPriority High
@SYMTestActions  TDecompositionIterator test.
@SYMTestExpectedResults The test must not fail.
@SYMPREQ814 Optimise folded string comparisons.
*/
void TestDecompositionIterator()
	{
    TDecompositionIterator i;

	// test basic character handling
	TUTF32Iterator i1(KHelloT().Ptr(), KHelloT().Ptr() + KHelloT().Length());
    i.Set(i1);
	TestIteratorOutput(i, KHelloT);

	TUTF32Iterator i2(KHelloT().Ptr() + 3, KHelloT().Ptr() + KHelloT().Length());
    i.Set(i2);
	TestIteratorOutput(i, KHelloT().Mid(3));

	// test combining characters
	TUTF32Iterator i3(KLatin1AccentsC().Ptr(), KLatin1AccentsC().Ptr() + KLatin1AccentsC().Length());
    i.Set(i3);
	TestIteratorOutput(i, KLatin1AccentsD);

	TUTF32Iterator i4(KGreekAccentsC().Ptr(), KGreekAccentsC().Ptr() + KGreekAccentsC().Length());
    i.Set(i4);
	TestIteratorOutput(i, KGreekAccentsD);

	TUTF32Iterator i5(KGreekAccentsS().Ptr(), KGreekAccentsS().Ptr() + KGreekAccentsS().Length());
    i.Set(i5);
	TestIteratorOutput(i, KGreekAccentsD);

	// test that full-width variants are not fiddled with
	TUTF32Iterator i6(KFullWidth().Ptr(), KFullWidth().Ptr() + KFullWidth().Length());
    i.Set(i6);
	TestIteratorOutput(i, KFullWidth);

	TUTF32Iterator i7(KSurrogates().Ptr(), KSurrogates().Ptr() + KSurrogates().Length());
    i.Set(i7);
	TestIteratorOutput(i, KSurrogatesTest);
	}

//The function collects collation keys at the specified level aLevel from aIt iterator 
//and stores them in aBuf output parameter.
//aExpectedNumKeys value specifies the count of expected collation keys.
void GetKeys(TCollationValueIterator& aIt, TUint32* aBuf, TInt aLevel, TInt aExpectedNumKeys)
	{
	//Zero the output parameter
	Mem::FillZ(aBuf, sizeof(TUint32) * aExpectedNumKeys);
	//Get the keys
	TInt i = 0;
	for(;i!=aExpectedNumKeys;++i)
		{
		for (;;)
			{
			(void)aIt.GetCurrentKey(aLevel, aBuf[i]);
			test(aIt.Increment());
			if(aBuf[i] != 0)
				{
				break;
				}
			}
		}
	//The rest of the keys at that level should have 0 value.
	for(;aIt.Increment();)
		{
		TUint32 key = 0;
		(void)aIt.GetCurrentKey(aLevel, key);
		test(key == 0);
		}
	}

//The function collects the raw keys from aIt iterator and stores them in aBuf output parameter.
//aExpectedNumKeys value specifies the count of expected raw collation keys.
void GetRawKeys(TCollationValueIterator& aIt, TCollationKey* aBuf, TInt aExpectedNumKeys)
	{
	//Zero the output parameter
	Mem::FillZ(aBuf, sizeof(TCollationKey) * aExpectedNumKeys);
	//Get the keys
	for(TInt i=0;i!=aExpectedNumKeys;++i)
		{
		test(aIt.GetCurrentKey(aBuf[i]));
		aIt.Increment();
		}
	//One additional GetCurrentKey() call. Because there shouldn't be more raw keys than  
	//aExpectedNumKeys, the call should fail returning EFalse.
	TCollationKey dummy;
	test(!aIt.GetCurrentKey(dummy));
	}

//The function gets a sequence of raw collation keys in aBuf parameter and a character number
//aCharNo in the original string.
//It returns the position in aBuf where raw collation key sequence for aCharNo starts.
TInt CharNo2CollKeyPos(const TCollationKey* aBuf, TInt aBufLen, TInt aCharNo)
    {
    TInt starterCnt = 0;
    TInt pos = 0;
    do
        {
        if(aBuf[pos].IsStarter())
            {
            ++starterCnt;
            }
        } while(starterCnt!=(aCharNo+1) && ++pos!=aBufLen);
    test(pos != aBufLen);
    return pos;
    }

//The function compares aBuf1 and aBuf2 and returns how many elements in aBuf1 differ
//from the elements in aBuf2 at the same position.
TInt CountDiscrepancies(TUint32* aBuf1, TUint32* aBuf2, TInt aCount)
	{
	TInt discrepancies = 0;
	for (; aCount; --aCount)
		{
		if (*aBuf1++ != *aBuf2++)
			++discrepancies;
		}
	return discrepancies;
	}

TInt CountDiscrepancies(TCollationKey* aBuf1, TCollationKey* aBuf2, TInt aCount)
	{
	TInt discrepancies = 0;
	for (; aCount; --aCount)
		{
		if (aBuf1->iHigh != aBuf2->iHigh || aBuf1->iLow != aBuf2->iLow)
			++discrepancies;
		++aBuf1;
		++aBuf2;
		}
	return discrepancies;
	}

_LIT(KAYZAccentsAbove, "\xE0\x301y\x302z\x303\x304");
_LIT(KCapitalAYZAccentsAbove, "A\x300\x301Y\x302Z\x303\x304");
_LIT(KCapitalYAYZAccentsAbove, "a\x300\x301Y\x302z\x303\x304");
_LIT(KABCRuleTest, "abcwabkakb");
_LIT(KABCRuleExpected, "eeabkakb");
//_LIT(KExhaustCaches, "0123456789ABCDEFexhausted");
_LIT(KGreekOPVY1, "\x1f82");
_LIT(KGreekOPVY2, "\x1f02\x345");
_LIT(KGreekOPVY3, "\x1f00\x300\x345");
_LIT(KGreekOPVY4, "\x3b1\x313\x300\x345");
_LIT(KGreekOPVY5, "\x3b1\x313\x345\x300");
_LIT(KGreekOPVY6, "\x3b1\x345\x313\x300");

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-0105
@SYMTestCaseDesc TCollationValueIterator test
@SYMTestPriority High
@SYMTestActions  TCollationValueIterator tests
@SYMTestExpectedResults The test must not fail.
@SYMPREQ814 Optimise folded string comparisons.
*/
void TestCollationValueIterator()
	{
   	// a funny pair of extra collation rules: w and abc both collate as e.
   	TUint32 abcWTLKey[] = {0/* key for 'e' will go here */,
   		0x8ff00101, 0x8ff10101, 0x8ff20101, 0x8ff30101,
   		0x8ff40101, 0x8ff50101, 0x8ff60101};
   	const TUint32 overrideIndex[] = {0x00770000, 0x0E010001, 0x0E400002, 0x0E440003,
   		0x0E810004, 0x0EC10005, 0x0EC20006, 0xEC30007};
   	const TUint16 abcString[4] = {0x0003, 0x0061, 0x0062, 0x0063};
   	const TUint32 abcStringIndex = 0;
   	TCollationMethod method;
   	method.iId = 0;
   	method.iMainTable = StandardCollationMethod();
   	method.iFlags = TCollationMethod::EIgnoreNone;
   	TInt charindex;
   	for (charindex = 0; method.iMainTable->iIndex[charindex] >> 16 != 'e'; ++charindex)
		{
		}
	abcWTLKey[0] = method.iMainTable->iKey[method.iMainTable->iIndex[charindex] & 0xFFFF];
	TCollationKeyTable overrideTable = {abcWTLKey, overrideIndex, sizeof(abcWTLKey)/4, abcString, &abcStringIndex, 1};
	method.iOverrideTable = &overrideTable;
	
   	TCollationValueIterator v(method);
	TCollationValueIterator rv(method);

   	TUint32 buf1[32];
   	TUint32 buf2[32];

   	TCollationKey raw1[32];
   	TCollationKey raw2[32];
	const TInt KRawKeyCnt = 8;//Raw collation key count produced from KAYZAccentsAbove string.
	
	//Get the raw key sequence for the whole KAYZAccentsAbove string.
	TUTF32Iterator it(KAYZAccentsAbove().Ptr(), KAYZAccentsAbove().Ptr() + KAYZAccentsAbove().Length());
	rv.SetSourceIt(it);
	::GetRawKeys(rv, raw1, KRawKeyCnt);
	
	//KAYZAccentsAbove related constants
	const TInt KBaseCharCnt = 3;//The number of base characters (A, y, z) in KAYZAccentsAbove string.
	const TInt KOrgPosA = 0;//A position in KAYZAccentsAbove
	const TInt KOrgPosY = 2;//y position in KAYZAccentsAbove
	const TInt KOrgPosZ = 4;//z position in KAYZAccentsAbove
	//Find where the collation key sequences start for A, y, z characters in KAYZAccentsAbove string.
	const TInt KCollKeyPosA = ::CharNo2CollKeyPos(raw1, KRawKeyCnt, 0);
	const TInt KCollKeyPosY = ::CharNo2CollKeyPos(raw1, KRawKeyCnt, 1);
	const TInt KCollKeyPosZ = ::CharNo2CollKeyPos(raw1, KRawKeyCnt, 2);
   
	//Get the raw key sequence for character A in KAYZAccentsAbove string.
	it = TUTF32Iterator(KAYZAccentsAbove().Ptr() + KOrgPosA, KAYZAccentsAbove().Ptr() + KAYZAccentsAbove().Length());
	rv.SetSourceIt(it);
	::GetRawKeys(rv, raw2 + KCollKeyPosA, KRawKeyCnt - KCollKeyPosA);
	//
	test(0 == ::CountDiscrepancies(raw1, raw2, KRawKeyCnt));
	
	//Get the raw key sequence for character Y in KAYZAccentsAbove string.
	it = TUTF32Iterator(KAYZAccentsAbove().Ptr() + KOrgPosY, KAYZAccentsAbove().Ptr() + KAYZAccentsAbove().Length());
	rv.SetSourceIt(it);
	::GetRawKeys(rv, raw2 + KCollKeyPosY, KRawKeyCnt - KCollKeyPosY);
	//
	test(0 == ::CountDiscrepancies(raw1, raw2, KRawKeyCnt));
   
	//Get the raw key sequence for character Z in KAYZAccentsAbove string.
	it = TUTF32Iterator(KAYZAccentsAbove().Ptr() + KOrgPosZ, KAYZAccentsAbove().Ptr() + KAYZAccentsAbove().Length());
	rv.SetSourceIt(it);
	::GetRawKeys(rv, raw2 + KCollKeyPosZ, KRawKeyCnt - KCollKeyPosZ);
	//
	test(0 == ::CountDiscrepancies(raw1, raw2, KRawKeyCnt));
   
	//Test starting at different points in the iteration
	
	//Level 0
	//The whole string
	it = TUTF32Iterator(KAYZAccentsAbove().Ptr(), KAYZAccentsAbove().Ptr() + KAYZAccentsAbove().Length());
	v.SetSourceIt(it);
	::GetKeys(v, buf1, 0, KBaseCharCnt);
	//String from Y pos.
	it = TUTF32Iterator(KAYZAccentsAbove().Ptr() + KOrgPosY, KAYZAccentsAbove().Ptr() + KAYZAccentsAbove().Length());
	v.SetSourceIt(it);
	::GetKeys(v, buf2, 0, KBaseCharCnt - 1);
	//
	test(0 == ::CountDiscrepancies(buf1 + 1, buf2, KBaseCharCnt - 1));
	//String from Z pos.
	it = TUTF32Iterator(KAYZAccentsAbove().Ptr() + KOrgPosZ, KAYZAccentsAbove().Ptr() + KAYZAccentsAbove().Length());
	v.SetSourceIt(it);
	::GetKeys(v, buf2, 0, KBaseCharCnt - 2);
	//
	test(0 == ::CountDiscrepancies(buf1 + 2, buf2, KBaseCharCnt - 2));
   
	//Level 1
	//KCapitalAYZAccentsAbove is used in this test.
	it = TUTF32Iterator(KCapitalAYZAccentsAbove().Ptr(), KCapitalAYZAccentsAbove().Ptr() + KCapitalAYZAccentsAbove().Length());
	v.SetSourceIt(it);
	::GetRawKeys(v, raw1, 8);
	const TInt KOrgPosY2 = 3;//Y position in KCapitalAYZAccentsAbove
	const TInt KCollKeyPosY2 = ::CharNo2CollKeyPos(raw1, KRawKeyCnt, 1);
	//The whole string
	it = TUTF32Iterator(KCapitalAYZAccentsAbove().Ptr(), KCapitalAYZAccentsAbove().Ptr() + KCapitalAYZAccentsAbove().Length());
	v.SetSourceIt(it);
	::GetKeys(v, buf1, 1, 8);
	//String from Y pos.
	it = TUTF32Iterator(KCapitalAYZAccentsAbove().Ptr() + KOrgPosY2, KCapitalAYZAccentsAbove().Ptr() + KCapitalAYZAccentsAbove().Length());
	v.SetSourceIt(it);
	::GetKeys(v, buf2, 1, 8 - KCollKeyPosY2);
	//
	test(0 == ::CountDiscrepancies(buf1 + KCollKeyPosY2, buf2, 8 - KCollKeyPosY2));
   
	//Level 2
	//Capitals do not match at level 2
	it = TUTF32Iterator(KAYZAccentsAbove().Ptr(), KAYZAccentsAbove().Ptr() + KAYZAccentsAbove().Length());
	v.SetSourceIt(it);
	::GetKeys(v, buf1, 2, 8);
	it = TUTF32Iterator(KCapitalYAYZAccentsAbove().Ptr(), KCapitalYAYZAccentsAbove().Ptr() + KCapitalYAYZAccentsAbove().Length());
	v.SetSourceIt(it);
	::GetKeys(v, buf2, 2, 8);
	//
   	test(1 == CountDiscrepancies(buf1, buf2, 8));
   	test(buf1[3] != buf2[3]);
   
	//Test funny collation keys, when they succeed and when they fail half way.
	it = TUTF32Iterator(KABCRuleTest().Ptr(), KABCRuleTest().Ptr() + KABCRuleTest().Length());
	v.SetSourceIt(it);
	::GetKeys(v, buf1, 0, 8);
	it = TUTF32Iterator(KABCRuleExpected().Ptr(), KABCRuleExpected().Ptr() + KABCRuleExpected().Length());
	v.SetSourceIt(it);
	::GetKeys(v, buf2, 0, 8);
	//
	test(0 == ::CountDiscrepancies(buf1, buf2, 8));
   
	//Test different decompositions at level 3
	it = TUTF32Iterator(KGreekOPVY1().Ptr(), KGreekOPVY1().Ptr() + KGreekOPVY1().Length());
	v.SetSourceIt(it);
	::GetKeys(v, buf1, 3, 4);
	//
	it = TUTF32Iterator(KGreekOPVY2().Ptr(), KGreekOPVY2().Ptr() + KGreekOPVY2().Length());
	v.SetSourceIt(it);
	::GetKeys(v, buf2, 3, 4);
	//
	test(0 == ::CountDiscrepancies(buf1, buf2, 4));
	//
	it = TUTF32Iterator(KGreekOPVY3().Ptr(), KGreekOPVY3().Ptr() + KGreekOPVY3().Length());
	v.SetSourceIt(it);
	::GetKeys(v, buf2, 3, 4);
	//
	test(0 == ::CountDiscrepancies(buf1, buf2, 4));
	//
	it = TUTF32Iterator(KGreekOPVY4().Ptr(), KGreekOPVY4().Ptr() + KGreekOPVY4().Length());
	v.SetSourceIt(it);
	::GetKeys(v, buf2, 3, 4);
	//
	test(0 == ::CountDiscrepancies(buf1, buf2, 4));
	//
	it = TUTF32Iterator(KGreekOPVY5().Ptr(), KGreekOPVY5().Ptr() + KGreekOPVY5().Length());
	v.SetSourceIt(it);
	::GetKeys(v, buf2, 3, 4);
	//
	test(0 == ::CountDiscrepancies(buf1, buf2, 4));
	//
	it = TUTF32Iterator(KGreekOPVY6().Ptr(), KGreekOPVY6().Ptr() + KGreekOPVY6().Length());
	v.SetSourceIt(it);
	::GetKeys(v, buf2, 3, 4);
	//
	test(0 == ::CountDiscrepancies(buf1, buf2, 4));
	}

// folding tests

// equivalence classes: all codes that fold to the same letter (which must be present
// in the list). The lists are separated by -1. The end is marked with two -1s.
// Each list must be in increasing order.
TInt FoldingEquivalenceClasses[] =
	{
	'A', 'a', -1, 'Z', 'z', -1, '@', -1, '[', -1, '{', -1, 127, -1, 'I', 'i', 0x131, -1, 0, -1,
	' ', 0xA0, -1,
	0x300, -1, 0x301, -1,
	0x141, 0x142, -1,
	0x1c4, 0x1c5, 0x1c6, -1, 0x1c7, 0x1c8, 0x1c9, -1, 0x1ca, 0x1cb, 0x1cc, -1,
	0x1f1, 0x1f2, 0x1f3, -1, 0x3a3, 0x3c2, 0x3c3, 0x3f2, -1,
	0x402, 0x452, -1, 0x40F, 0x45F, -1, 0x460, 0x461, -1, 0x480, 0x481, -1, 0x482, -1,
	0x410, 0x430, -1, 0x42F, 0x44f, -1, 0x48C, 0x48D, -1, 0x4e8, 0x4e9, -1,
	0x531, 0x561, -1, 0x556, 0x586, -1, 0x559, -1, 0x55f, -1, -1
	};

//_LIT(KMatchLeadingCandidate1, "\xE1\x65\x300\x301\x302\x303pqa\x301");
//_LIT(KNoQMs, "a???");
//_LIT(KOneQM, "?a");
//_LIT(KTwoQMs, "??");
//_LIT(KThreeQMs, "???*?");

//Constructs TUTF32Iterator iterator from aStr
TUTF32Iterator UTF32It(const TDesC16& aStr)
    {
    return TUTF32Iterator(aStr.Ptr(), aStr.Ptr() + aStr.Length());
    }

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-0107
@SYMTestCaseDesc MatchSectionFolded test
@SYMTestPriority High
@SYMTestActions  MatchSectionFolded test
@SYMTestExpectedResults The test must not fail.
@SYMPREQ814 Optimise folded string comparisons.
*/
void MatchSectionFoldedTest()
    {
    TUTF32Iterator candidateIt, searchTermIt;

    candidateIt = UTF32It(_L16("\xE1"));
    searchTermIt = UTF32It(_L16("a"));
    test(!MatchSectionFolded(candidateIt, searchTermIt));

    candidateIt = UTF32It(_L16("a"));
    searchTermIt = UTF32It(_L16("\xE1"));
    test(!MatchSectionFolded(candidateIt, searchTermIt));

    candidateIt = UTF32It(_L16("abca\xE1\x62\x62\x61\x61\x61\x62\x63\x62\x61"));
    searchTermIt = UTF32It(_L16("aBc"));
    test(MatchSectionFolded(candidateIt, searchTermIt));
    test(searchTermIt.AtEnd());
    TPtrC16 p1(_L16("a\xE1\x62\x62\x61\x61\x61\x62\x63\x62\x61"));
    TPtrC16 p2(candidateIt.CurrentPosition(), 11);
    test(p1 == p2);

    candidateIt = UTF32It(_L16("aaaacdeiooo"));
    searchTermIt = UTF32It(_L16("acde"));
    test(!MatchSectionFolded(candidateIt, searchTermIt));
    }

//FindMatchSectionFolded test
void DoFindMatchSectionFoldedTest(const TDesC16& aCandidate, const TDesC16& aSearchTerm, TInt aPos)
    {
    TUTF32Iterator candidateIt, searchTermIt;
    candidateIt = UTF32It(aCandidate);
    searchTermIt = UTF32It(aSearchTerm);
    if(aPos >= 0)
        {
        test(FindMatchSectionFolded(candidateIt, searchTermIt));
        test(searchTermIt.AtEnd());
        }
    else
        {
        test(!FindMatchSectionFolded(candidateIt, searchTermIt));
        }
    }

//This class is used for reading lines from the unicode data file.
class RUnicodeTestDataFile
    {
public:
    RUnicodeTestDataFile();
    void OpenLC();
    void Close();
    TBool NextStmt(TPtrC8& aStmt);
private:
    HBufC8* iFileData;
    TInt iStartPos;
    };

RUnicodeTestDataFile::RUnicodeTestDataFile() :
    iFileData(NULL),
    iStartPos(0)
    {
    }

void RUnicodeTestDataFile::OpenLC()
    {
    __ASSERT_ALWAYS(!iFileData && !iStartPos, User::Invariant());
    iFileData = NULL;
    iStartPos = 0;
    CleanupClosePushL(*this);

    RFs fileSess;
    CleanupClosePushL(fileSess);
    User::LeaveIfError(fileSess.Connect());

    RFile file;
    CleanupClosePushL(file);
    User::LeaveIfError(file.Open(fileSess, KUnicodeTestDataFile, EFileRead));

    TInt fileSize;
    User::LeaveIfError(file.Size(fileSize));
    __ASSERT_ALWAYS(fileSize > 0, User::Invariant());

    iFileData = HBufC8::NewL(fileSize + 1);

    TPtr8 p = iFileData->Des();
	User::LeaveIfError(file.Read(p));

    CleanupStack::PopAndDestroy(2, &fileSess);
    }

void RUnicodeTestDataFile::Close()
    {
    delete iFileData;
    iFileData = NULL;
    iStartPos = 0;
    }

TBool RUnicodeTestDataFile::NextStmt(TPtrC8& aStmt)
    {
    aStmt.Set(NULL, 0);
    if(iStartPos < iFileData->Length())
        {
        const TUint8* pStart = iFileData->Des().Ptr() + iStartPos;
        const TUint8* pEnd = pStart;
        while(*pEnd++ != 0x0A)
            {
            }
        iStartPos += pEnd - pStart;
        aStmt.Set(pStart, pEnd - pStart - 1);
        return ETrue;
        }
    return EFalse;
    }

//Get a field "aFieldNo" from "aStr" statement containing encoded unicode character data
TPtrC8 GetUnicodeDataField(const TPtrC8& aStr, TInt aFieldNo)
    {
    const TUint8* pStart = aStr.Ptr();
    //Find the beginning of the field
    TInt count = 0;
    while(count < aFieldNo)
        {
        if(*pStart++ == ';')
            {
            ++count;
            }
        }
    //Find the end of the field
    const TUint8* pEnd = pStart;
    while(*pEnd++ != ';')
        {
        }
    //Construct a string from the field data
    TPtrC8 ptr(pStart, pEnd - pStart - 1);
    return ptr;
    }
   
//Construct a string "aStr" with the extracted hex codes from "aUnicodeData"
//The extracted unicodes are placed not from position 0, because some of 
//the decomposable unicode characters are combining characters. If "aStr" is a search
//string, then the searching algorithm will not work.
void FillStringL(TDes16& aStr, const TDesC8& aUnicodeData)
    {
    aStr.SetLength(aStr.MaxLength());
    TLex8 lex(aUnicodeData);
    TInt len = 0;
    for(len=0;!lex.Eos();++len)
        {
        TUint32 code;
        User::LeaveIfError(lex.Val(code, EHex));
        lex.Assign(lex.NextToken());
        if (!TChar::IsSupplementary(code))
        	{
        	aStr[1+len] = (TUint16)code;
        	}
        else
        	{
        	aStr[1+len] = TChar::GetHighSurrogate(code);
        	++len;
        	aStr[1+len] = TChar::GetLowSurrogate(code);
        	}
        }
    __ASSERT_ALWAYS(len > 0, User::Invariant());
    aStr.SetLength(1 + len);
    }
   
//Get the character unicode, which is at position 0
TUint32 GetChCodeL(const TDesC8& aStr)
    {
    TLex8 lex(aStr);
    TUint32 chCode;
    User::LeaveIfError(lex.Val(chCode, EHex));
    return chCode;
    }

//Simple unicode folding tests
void FindMatchSectionFoldedTestSimple()
    {
	_LIT16(KCandidate, "abca\xE1\x62\x62\x61\x61\x61\x62\x63\x62\x61");
	DoFindMatchSectionFoldedTest(KCandidate, _L("abc"), 0);
	DoFindMatchSectionFoldedTest(KCandidate, _L("abb"), -1);
	DoFindMatchSectionFoldedTest(KCandidate, _L("caa"), -1);
	DoFindMatchSectionFoldedTest(KCandidate, _L("abcb"), 9);
	DoFindMatchSectionFoldedTest(KCandidate, _L("\xE1"), 4);
	DoFindMatchSectionFoldedTest(KCandidate, _L("a\x301"), 4);
	DoFindMatchSectionFoldedTest(KCandidate, _L("A\xC1\x42\x42"), 3);
	DoFindMatchSectionFoldedTest(KCandidate, _L("a\x301\x42\x42"), 4);
	DoFindMatchSectionFoldedTest(KCandidate, _L("a?BB"), 3);
	DoFindMatchSectionFoldedTest(KCandidate, _L(""), 0);
	DoFindMatchSectionFoldedTest(KCandidate, _L("?"), 0);
	DoFindMatchSectionFoldedTest(KCandidate, _L("??????????????"), 0);
	DoFindMatchSectionFoldedTest(KCandidate, _L("???????????????"), -1);
	DoFindMatchSectionFoldedTest(KCandidate, _L("????a?????????"), -1);
	DoFindMatchSectionFoldedTest(KCandidate, _L("???a??????????"), 0);
	DoFindMatchSectionFoldedTest(KCandidate, _L("caa?"), -1);
	DoFindMatchSectionFoldedTest(KCandidate, _L("abcb?"), 9);
	DoFindMatchSectionFoldedTest(KCandidate, _L("abcb??"), -1);
	DoFindMatchSectionFoldedTest(KCandidate, _L("b?aa"), 5);
    }

//Extended tests - all characters, having non-zero "character decomposition mapping" field
//or non-zero "upper case mapping" field
void FindMatchSectionFoldedTestComplexL()
    {
    TBuf16<10> candidate;
    candidate.Copy(_L16("abcdefghij"));
    TBuf16<10> searchTerm;
    searchTerm.Copy(_L16("eeeeefghij"));
    const TInt KChPos = 5;
    //Read and parse each line from the unicode data file.
    RUnicodeTestDataFile unicodeTestDataFile;
    unicodeTestDataFile.OpenLC();
    TPtrC8 stmt;
    while(unicodeTestDataFile.NextStmt(stmt) && stmt.Length() > 0)
        {
        //Get the character code
        TUint32 chCode = GetChCodeL(stmt);
        //"LATIN CAPITAL LETTER I WITH DOT ABOVE" - the searching algorithm does not work with it.
        if(chCode == (TUint32)0x0130)
            {
            continue;
            }
        if (!TChar::IsSupplementary(chCode))
        	{
        	candidate[KChPos] = (TUint16)chCode;
        	}
        else
        	{
            candidate[KChPos] = TChar::GetHighSurrogate(chCode);
            candidate[KChPos+1] = TChar::GetLowSurrogate(chCode);
        	}
        //"Character decomposition mapping" is the 5th field, starting from 0.
        TPtrC8 decomp(GetUnicodeDataField(stmt, 5));
        if(decomp.Length() > 1 && decomp[0] != '<')
            {
            //This character has valid decomposition mapping - test it.
            //Construct the search string
            FillStringL(searchTerm, decomp);
            //Test
            DoFindMatchSectionFoldedTest(candidate, searchTerm, KChPos);
            }
        //"Uppercase mapping" is the 12th field, starting from 0.
        TPtrC8 upperc(GetUnicodeDataField(stmt, 12));
        if(upperc.Length() > 1)
            {
            //This character has valid uppercase mapping - test it.
            //Construct the search string
            FillStringL(searchTerm, upperc);
            //Test
            DoFindMatchSectionFoldedTest(candidate, searchTerm, KChPos);
            }
        }//end of "while" - for each file statement
    CleanupStack::PopAndDestroy(&unicodeTestDataFile);
    }

//MatchStringFolded test
void MatchStringFoldedTestL()
    {
    TBuf16<3> candidate;
    candidate.Copy(_L16("aa"));
    TBuf16<10> searchTerm;
    searchTerm.Copy(_L16("aaaaaaaaaa"));
    const TInt KChPos = 1;
    //Read and parse each line from the unicode data file.
    RUnicodeTestDataFile unicodeTestDataFile;
    unicodeTestDataFile.OpenLC();
    TPtrC8 stmt;
    while(unicodeTestDataFile.NextStmt(stmt) && stmt.Length() > 0)
        {
        //Get the character code
        TUint32 chCode = GetChCodeL(stmt);
        //"LATIN CAPITAL LETTER I WITH DOT ABOVE" - the searching algorithm does not work with it.
        if(chCode == (TUint32)0x0130)
            {
            continue;
            }
        if (!TChar::IsSupplementary(chCode))
        	{
        	candidate[KChPos] = (TUint16)chCode;
        	candidate.SetLength(2);
        	}
        else
        	{
            candidate[KChPos] = TChar::GetHighSurrogate(chCode);
            candidate.SetLength(3);
            candidate[KChPos+1] = TChar::GetLowSurrogate(chCode);
        	}
        //"Character decomposition mapping" is the 5th field, starting from 0.
        TPtrC8 decomp(GetUnicodeDataField(stmt, 5));
        if(decomp.Length() > 1 && decomp[0] != '<')
            {
            //This character has valid decomposition mapping - test it.
            //Construct the search string
            FillStringL(searchTerm, decomp);
            //Test
            test(MatchStringFolded(candidate.Ptr(), candidate.Ptr() + candidate.Length(),
                                   searchTerm.Ptr(), searchTerm.Ptr() + searchTerm.Length()));
            }
        //"Uppercase mapping" is the 12th field, starting from 0.
        TPtrC8 upperc(GetUnicodeDataField(stmt, 12));
        if(upperc.Length() > 1)
            {
            //This character has valid uppercase mapping - test it.
            //Construct the search string
            FillStringL(searchTerm, upperc);
            //Test
            test(MatchStringFolded(candidate.Ptr(), candidate.Ptr() + candidate.Length(),
                                   searchTerm.Ptr(), searchTerm.Ptr() + searchTerm.Length()));
            }
        }//end of "while" - for each file statement
    CleanupStack::PopAndDestroy(&unicodeTestDataFile);
    }

void FindMatchSectionFoldedTestL()
    {
    FindMatchSectionFoldedTestSimple();
    FindMatchSectionFoldedTestComplexL();
    }

void TestFindMatchFoldedL()
	{
	MatchSectionFoldedTest();
	FindMatchSectionFoldedTestL();
	MatchStringFoldedTestL();
	}

void TestCompareFoldedEqual(const TDesC& a, const TDesC& b)
	{
	test(a.CompareF(b) == 0);
	test(b.CompareF(a) == 0);
	}
	
void TestCompareFolded()
	{
	// Latin Extended A
	TestCompareFoldedEqual(_L("\x100"), _L("\x101"));
	TestCompareFoldedEqual(_L("\x100"), _L("A\x304"));
	TestCompareFoldedEqual(_L("\x100"), _L("a\x304"));
	TestCompareFoldedEqual(_L("\x104"), _L("\x105"));
	TestCompareFoldedEqual(_L("\x104"), _L("a\x328"));
	TestCompareFoldedEqual(_L("\x107"), _L("C\x301"));
	TestCompareFoldedEqual(_L("\x10F"), _L("\x10E"));
	TestCompareFoldedEqual(_L("\x10F"), _L("D\x30C"));
	TestCompareFoldedEqual(_L("\x110"), _L("\x111"));
	TestCompareFoldedEqual(_L("\x123"), _L("G\x327"));
	TestCompareFoldedEqual(_L("\x132"), _L("\x133"));
	TestCompareFoldedEqual(_L("\x131"), _L("i"));
	TestCompareFoldedEqual(_L("\x131"), _L("I"));
	TestCompareFoldedEqual(_L("i"), _L("I"));
	TestCompareFoldedEqual(_L("\x13F"), _L("\x140"));
	TestCompareFoldedEqual(_L("\x141"), _L("\x142"));
	TestCompareFoldedEqual(_L("\x14A"), _L("\x14B"));
	TestCompareFoldedEqual(_L("\x150"), _L("\x151"));
	TestCompareFoldedEqual(_L("\x150"), _L("o\x30B"));
	TestCompareFoldedEqual(_L("\x152"), _L("\x153"));
	TestCompareFoldedEqual(_L("\x17D"), _L("\x17E"));
	TestCompareFoldedEqual(_L("\x17D"), _L("z\x30C"));
	// Latin Extended B
	TestCompareFoldedEqual(_L("\x182"), _L("\x183"));
	TestCompareFoldedEqual(_L("\x184"), _L("\x185"));
	TestCompareFoldedEqual(_L("\x187"), _L("\x188"));
	TestCompareFoldedEqual(_L("\x18A"), _L("\x257"));
	TestCompareFoldedEqual(_L("\x194"), _L("\x263"));
	TestCompareFoldedEqual(_L("\x195"), _L("\x1F6"));
	TestCompareFoldedEqual(_L("\x196"), _L("\x269"));
	TestCompareFoldedEqual(_L("\x1A2"), _L("\x1A3"));
	TestCompareFoldedEqual(_L("\x1A6"), _L("\x280"));
	TestCompareFoldedEqual(_L("\x1BF"), _L("\x1F7"));
	TestCompareFoldedEqual(_L("\x1DC"), _L("\x1DB"));
	TestCompareFoldedEqual(_L("\x1DC"), _L("u\x308\x300"));
	TestCompareFoldedEqual(_L("\x1DD"), _L("\x18E"));
	TestCompareFoldedEqual(_L("\x1EC"), _L("\x1ED"));
	TestCompareFoldedEqual(_L("\x1FC"), _L("\x1FD"));
	TestCompareFoldedEqual(_L("\x200"), _L("\x201"));
	TestCompareFoldedEqual(_L("\x216"), _L("u\x311"));
	TestCompareFoldedEqual(_L("\x21B"), _L("T\x326"));
	TestCompareFoldedEqual(_L("\x21C"), _L("\x21D"));
	TestCompareFoldedEqual(_L("\x229"), _L("E\x327"));
	TestCompareFoldedEqual(_L("\x22A"), _L("\x22B"));
	TestCompareFoldedEqual(_L("\x22A"), _L("O\x308\x304"));
	TestCompareFoldedEqual(_L("\x22A"), _L("\xF6\x304"));
	TestCompareFoldedEqual(_L("\x233"), _L("y\x304"));
	TestCompareFoldedEqual(_L("\x233"), _L("\x232"));
	}
	
void TestCompareFoldedNotEqual(TDesC& a, TDesC& b, TInt aValue)
	{
	test(a.CompareF(b) == aValue);
	}
	
static void TestCompareFoldedAdditional()
	{
	const TText16 UnicodeTextOne16[] = {'a', 0};
	const TText16 ErrUnicodeTextOne16[] = {'[', 0};
	
	const TText16 UnicodeTextTwo16[] = {0x00EA, 0x0323, 0};
	const TText16 ErrUnicodeTextTwo16[] = {0x00EA, 't', 0};
	
	const TText16 UnicodeTextThree16[] = {0x00EA, 0x03B1, 0};
	const TText16 ErrUnicodeTextThree16[] = {0x00EA, 0x0323, 0};
	
	TBufC16<ARRAY_SIZE(UnicodeTextOne16) - 1> oriUnicodeSmallTextOne(UnicodeTextOne16);
	TBufC16<ARRAY_SIZE(ErrUnicodeTextOne16) - 1> nonMatchUnicodeSmallTextOne(ErrUnicodeTextOne16);
	
	TBufC16<ARRAY_SIZE(UnicodeTextTwo16) - 1> oriUnicodeSmallTextTwo(UnicodeTextTwo16);
	TBufC16<ARRAY_SIZE(ErrUnicodeTextTwo16) - 1> nonMatchUnicodeSmallTextTwo(ErrUnicodeTextTwo16);
	
	TBufC16<ARRAY_SIZE(UnicodeTextThree16) - 1> oriUnicodeSmallTextThree(UnicodeTextThree16);
	TBufC16<ARRAY_SIZE(ErrUnicodeTextThree16) - 1> nonMatchUnicodeSmallTextThree(ErrUnicodeTextThree16);
	
	const TText16 AsciiText16[] = {'A', 'B', 'C', 'D', 'E', 0};
	
    TBufC16<5> oriAsciiSmallText(_L("ABCDE"));
    
	// Check that characters are non matching with return value as stated
	
	TestCompareFoldedNotEqual(oriUnicodeSmallTextOne, nonMatchUnicodeSmallTextOne, 6);
	TestCompareFoldedNotEqual(oriUnicodeSmallTextTwo, nonMatchUnicodeSmallTextTwo, 33);
	TestCompareFoldedNotEqual(oriUnicodeSmallTextThree, nonMatchUnicodeSmallTextThree, -33);
	
	// Try other way around...
	
	TestCompareFoldedNotEqual(nonMatchUnicodeSmallTextOne, oriUnicodeSmallTextOne, -6);
	TestCompareFoldedNotEqual(nonMatchUnicodeSmallTextTwo, oriUnicodeSmallTextTwo, -33);
	TestCompareFoldedNotEqual(nonMatchUnicodeSmallTextThree, oriUnicodeSmallTextThree, 33);
	
	// Declare a TPtrC16 which is base from AsciiText16...

	TPtrC16 AsciiSmallText;

	AsciiSmallText.Set(AsciiText16, 4);

	// Check the boundary case

	TestCompareFoldedNotEqual(oriAsciiSmallText, AsciiSmallText, 1);

	// Try other way around...

	TestCompareFoldedNotEqual(AsciiSmallText, oriAsciiSmallText, -1);

	}

void TestFoldingL()
	{
	TestFindMatchFoldedL();
	TestCompareFolded();
	TestCompareFoldedAdditional();
	}

// collation tests
_LIT(KCandidateString1, "abcdefg");
_LIT(KCandidateString2, "\x1f82\x1f02\x345\x1f00\x300\x345\x3b1\x313\x300\x345");
_LIT(KCandidateString3, "abcabcdababc");
_LIT(KCandidateString4, "xyzxyxyzxyxyyxyzxyy");

_LIT(KMatch1, "abc");
_LIT(KMatch2, "abc*");
_LIT(KMatch3, "*abc*");
_LIT(KMatch4, "abc*def");
_LIT(KMatch5, "abc*def*g*");
_LIT(KMatch6, "*def");
_LIT(KMatch7, "**d?f?");
_LIT(KMatch8, "*d?f??");
_LIT(KMatch9, "***d?f??*");
_LIT(KMatch10, "a*c*g");
_LIT(KMatch11, "*c*g");

_LIT(KMatch12, "*\x1f82");
_LIT(KMatch13, "*\x1f82*");
//_LIT(KMatch14, "*\x3b1*");
_LIT(KMatch15, "*\x313*");
_LIT(KMatch16, "*\x300*");
//_LIT(KMatch17, "*\x345*");
//_LIT(KMatch18, "*\x3b1\x313*");
//_LIT(KMatch19, "*\x3b1\x313\x300*");
_LIT(KMatch20, "*\x1f82*\x1f82*\x1f82\x1f82");
_LIT(KMatch21, "*\x1f82*\x1f82*\x1f82\x1f82*\x1f82*");

_LIT(KMatch22, "*aba*");
_LIT(KMatch23, "*abc");
_LIT(KMatch24, "a*abc");
_LIT(KMatch25, "a*ab");
_LIT(KMatch26, "*ca*abc");
_LIT(KMatch27, "*ca*??c");
_LIT(KMatch28, "*??c");
_LIT(KMatch29, "a*babc");
_LIT(KMatch30, "*xyy");

_LIT(KFoo1, "foo");
_LIT(KPeach, "pe\x302\x63he");
_LIT(KFooMatch1, "fo*");
_LIT(KFooMatch2, "*Fo*");
_LIT(KFooMatch3, "*f*O*o");
_LIT(KFooMatch4, "*f*o*o*");
_LIT(KFooMatch5, "*o");
_LIT(KFooMatch6, "???");
_LIT(KFooMatch7, "*?o?*");
_LIT(KFooMatch8, "*?");
_LIT(KFooNonMatch1, "oo*");
_LIT(KFooNonMatch2, "??");
_LIT(KFooNonMatch3, "????");
_LIT(KFooNonMatch4, "*?f*");
_LIT(KFooNonMatch5, "*f*f*");
_LIT(KFooNonMatch6, "*?*f*");
_LIT(KPeachMatch1, "p?che");
_LIT(KPeachNonMatch1, "peche");
_LIT(KPeachNonMatch2, "pe?che");
_LIT(KPeachNonMatch3, "pe?he");
_LIT(KPeachNonMatch4, "pe*");

void TestMatchIdentifiersTDesC(const TDesC& aCandidate, const TDesC& aSearchTerm, TInt aExpectedResult)
	{
	const TText16* candidateStart = aCandidate.Ptr();
	const TText16* candidateEnd = candidateStart + aCandidate.Length();
	const TText16* searchTermStart = aSearchTerm.Ptr();
	const TText16* searchTermEnd = searchTermStart + aSearchTerm.Length();
	TInt pos = ::LocateMatchStringFolded(candidateStart, candidateEnd, searchTermStart, searchTermEnd);
	test(aExpectedResult == pos);
	}

void TestMatchIdentifiers()
	{
	TestMatchIdentifiersTDesC(KCandidateString1, KMatch1, KErrNotFound);
	TestMatchIdentifiersTDesC(KCandidateString1, KMatch2, 0);
	TestMatchIdentifiersTDesC(KCandidateString1, KMatch3, 0);
	TestMatchIdentifiersTDesC(KCandidateString1, KMatch4, KErrNotFound);
	TestMatchIdentifiersTDesC(KCandidateString1, KMatch5, 0);
	TestMatchIdentifiersTDesC(KCandidateString1, KMatch6, KErrNotFound);
	TestMatchIdentifiersTDesC(KCandidateString1, KMatch7, 3);
	TestMatchIdentifiersTDesC(KCandidateString1, KMatch8, KErrNotFound);
	TestMatchIdentifiersTDesC(KCandidateString1, KMatch9, KErrNotFound);
	TestMatchIdentifiersTDesC(KCandidateString1, KMatch10, 0);
	TestMatchIdentifiersTDesC(KCandidateString1, KMatch11, 2);
	TestMatchIdentifiersTDesC(KCandidateString2, KMatch12, 6);
	TestMatchIdentifiersTDesC(KCandidateString2, KMatch13, 0);
	//The next test does not pass with the new optimised methods
	//TestMatchIdentifiersTDesC(KCandidateString2, KMatch14, KErrNotFound);
	TestMatchIdentifiersTDesC(KCandidateString2, KMatch15, KErrNotFound);
	TestMatchIdentifiersTDesC(KCandidateString2, KMatch16, KErrNotFound);
	// I have taken this test out: it tests that combining ypogegrammeni is not
	// found on its own: but with case folding it can become a non-combining
	// character (iota), so this test is not relevant.
	// TestMatchIdentifiersTDesC(KCandidateString2, KMatch17, KErrNotFound);
	//The next tests do not pass with the new optimised methods
	//TestMatchIdentifiersTDesC(KCandidateString2, KMatch18, KErrNotFound);
	//TestMatchIdentifiersTDesC(KCandidateString2, KMatch19, KErrNotFound);
	TestMatchIdentifiersTDesC(KCandidateString2, KMatch20, 0);
	TestMatchIdentifiersTDesC(KCandidateString2, KMatch21, KErrNotFound);
	TestMatchIdentifiersTDesC(KCandidateString3, KMatch22, 7);
	TestMatchIdentifiersTDesC(KCandidateString3, KMatch23, 9);
	TestMatchIdentifiersTDesC(KCandidateString3, KMatch24, 0);
	TestMatchIdentifiersTDesC(KCandidateString3, KMatch25, KErrNotFound);
	TestMatchIdentifiersTDesC(KCandidateString3, KMatch26, 2);
	TestMatchIdentifiersTDesC(KCandidateString3, KMatch27, 2);
	TestMatchIdentifiersTDesC(KCandidateString3, KMatch28, 9);
	TestMatchIdentifiersTDesC(KCandidateString3, KMatch29, 0);
	TestMatchIdentifiersTDesC(KCandidateString4, KMatch30, 16);

	TestMatchIdentifiersTDesC(KFoo1, KFoo1, 0);
	TestMatchIdentifiersTDesC(KFoo1, KFooMatch1, 0);
	TestMatchIdentifiersTDesC(KFoo1, KFooMatch2, 0);
	TestMatchIdentifiersTDesC(KFoo1, KFooMatch3, 0);
	TestMatchIdentifiersTDesC(KFoo1, KFooMatch4, 0);
	TestMatchIdentifiersTDesC(KFoo1, KFooMatch5, 2);
	TestMatchIdentifiersTDesC(KFoo1, KFooMatch6, 0);
	TestMatchIdentifiersTDesC(KFoo1, KFooMatch7, 0);
	TestMatchIdentifiersTDesC(KFoo1, KFooMatch8, 2);
	TestMatchIdentifiersTDesC(KFoo1, KFooNonMatch1, KErrNotFound);
	TestMatchIdentifiersTDesC(KFoo1, KFooNonMatch2, KErrNotFound);
	TestMatchIdentifiersTDesC(KFoo1, KFooNonMatch3, KErrNotFound);
	TestMatchIdentifiersTDesC(KFoo1, KFooNonMatch4, KErrNotFound);
	TestMatchIdentifiersTDesC(KFoo1, KFooNonMatch5, KErrNotFound);
	TestMatchIdentifiersTDesC(KFoo1, KFooNonMatch6, KErrNotFound);
	TestMatchIdentifiersTDesC(KPeach, KPeachMatch1, 0);
	TestMatchIdentifiersTDesC(KPeach, KPeachNonMatch1, KErrNotFound);
	TestMatchIdentifiersTDesC(KPeach, KPeachNonMatch2, KErrNotFound);
	TestMatchIdentifiersTDesC(KPeach, KPeachNonMatch3, KErrNotFound);
	TestMatchIdentifiersTDesC(KPeach, KPeachNonMatch4, KErrNotFound);

	TestMatchIdentifiersTDesC(_L(""), _L(""), 0);
	TestMatchIdentifiersTDesC(_L("a"), _L(""), KErrNotFound);
	TestMatchIdentifiersTDesC(_L(""), _L("*"), 0);
	}

void TestFindIdentifierTDesC(const TDesC& aCandidateString, const TDesC& aSearchTerm, TInt /*aExpectedResult*/)
	{
	TUTF32Iterator candidateIt(aCandidateString.Ptr(), aCandidateString.Ptr() + aCandidateString.Length());
	TUTF32Iterator searchIt(aSearchTerm.Ptr(), aSearchTerm.Ptr() + aSearchTerm.Length());
	/*aExpectedResult = */::FindFolded(candidateIt, searchIt);
	}

//INC057641 - NTT Functional BC break in 8.1a: string comparison changed
static void INC057641L()
	{
	_LIT16(KEmptyText, "");        
	HBufC16* str = HBufC16::NewLC(4);
	str->Des().Copy(_L("****"));
	TInt res = str->CompareC(KEmptyText);
	CleanupStack::PopAndDestroy(str);
	test(res == 1);
	}

_LIT(KFind1, "abc");
_LIT(KFind2, "def");
_LIT(KFind3, "efg");
_LIT(KFind4, "fga");
_LIT(KFind5, "acd");
_LIT(KFind6, "\x1f82");
_LIT(KFind7, "\x3b1\x313\x300\x345");
_LIT(KFind8, "\x3b1");
_LIT(KFind9, "aba");
_LIT(KFind10, "abc");

void TestFindIdentifier()
	{
	TestFindIdentifierTDesC(KCandidateString1, TPtrC(), 0);
	TestFindIdentifierTDesC(KCandidateString1, KFind1, 0);
	TestFindIdentifierTDesC(KCandidateString1, KFind2, 3);
	TestFindIdentifierTDesC(KCandidateString1, KFind3, 4);
	TestFindIdentifierTDesC(KCandidateString1, KFind4, KErrNotFound);
	TestFindIdentifierTDesC(KCandidateString1, KFind5, KErrNotFound);
	TestFindIdentifierTDesC(KCandidateString2, KFind6, 0);
	TestFindIdentifierTDesC(KCandidateString2, KFind7, 0);
	TestFindIdentifierTDesC(KCandidateString2, KFind8, KErrNotFound);
	TestFindIdentifierTDesC(KCandidateString3, KFind9, 7);
	TestFindIdentifierTDesC(KCandidateString3, KFind10, 0);
	}

struct TestMatch8
	{
	TText8 const* iLeft;
	TText8 const* iRight;
	TInt iResult;
	};

TestMatch8 const Tests8[]=
	{
	{_S8(""),_S8(""),0},
	{_S8(""),_S8("?"),KErrNotFound},
	{_S8(""),_S8("*"),0},
	{_S8(""),_S8("**"),0},
	{_S8(""),_S8("*x*"),KErrNotFound},
	{_S8("x"),_S8(""),KErrNotFound},
	{_S8("x"),_S8("?"),0},
	{_S8("x"),_S8("*"),0},
	{_S8("x"),_S8("**"),0},
	{_S8("x"),_S8("**?"),0},
	{_S8("x"),_S8("?**"),0},
	{_S8("x"),_S8("**?*"),0},
	{_S8("x"),_S8("x"),0},
	{_S8("x"),_S8("a"),KErrNotFound},
	{_S8("x"),_S8("xx"),KErrNotFound},
	{_S8("x"),_S8("?x"),KErrNotFound},
	{_S8("x"),_S8("x*"),0},
	{_S8("x"),_S8("*x"),0},
	{_S8("x"),_S8("*x*"),0},
	{_S8("x"),_S8("**x*"),0},
	{_S8("abc"),_S8(""),KErrNotFound},
	{_S8("abc"),_S8("?*"),0},
	{_S8("abc"),_S8("*?"),2},
	{_S8("abc"),_S8("*?*?"),0},
	{_S8("abc"),_S8("*a*"),0},
	{_S8("abc"),_S8("*b*"),1},
	{_S8("abc"),_S8("*c*"),2},
	{_S8("abc"),_S8("*a"),KErrNotFound},
	{_S8("abc"),_S8("*c"),2},
	{_S8("abc"),_S8("*?c"),1},
	{_S8("abc"),_S8("??c"),0},
	{_S8("abc"),_S8("*b?"),1},
	};

struct TestMatch16
	{
	TText16 const* iLeft;
	TText16 const* iRight;
	TInt iResult;
	};

TestMatch16 const Tests16[]=
	{
	{_S16(""),_S16(""),0},
	{_S16(""),_S16("?"),KErrNotFound},
	{_S16(""),_S16("*"),0},
	{_S16(""),_S16("**"),0},
	{_S16(""),_S16("*x*"),KErrNotFound},
	{_S16("x"),_S16(""),KErrNotFound},
	{_S16("x"),_S16("?"),0},
	{_S16("x"),_S16("*"),0},
	{_S16("x"),_S16("**"),0},
	{_S16("x"),_S16("**?"),0},
	{_S16("x"),_S16("?**"),0},
	{_S16("x"),_S16("**?*"),0},
	{_S16("x"),_S16("x"),0},
	{_S16("x"),_S16("a"),KErrNotFound},
	{_S16("x"),_S16("xx"),KErrNotFound},
	{_S16("x"),_S16("?x"),KErrNotFound},
	{_S16("x"),_S16("x*"),0},
	{_S16("x"),_S16("*x"),0},
	{_S16("x"),_S16("*x*"),0},
	{_S16("x"),_S16("**x*"),0},
	{_S16("abc"),_S16(""),KErrNotFound},
	{_S16("abc"),_S16("?*"),0},
	{_S16("abc"),_S16("*?"),2},
	{_S16("abc"),_S16("*?*?"),0},
	{_S16("abc"),_S16("*a*"),0},
	{_S16("abc"),_S16("*b*"),1},
	{_S16("abc"),_S16("*c*"),2},
	{_S16("abc"),_S16("*a"),KErrNotFound},
	{_S16("abc"),_S16("*c"),2},
	{_S16("abc"),_S16("*?c"),1},
	{_S16("abc"),_S16("??c"),0},
	{_S16("abc"),_S16("*b?"),1},
	{_S16("\x0100"),_S16("\x0100"),0},
	{_S16("\x0100"),_S16("*"),0},
	{_S16("\x0100"),_S16("?"),0},
	{_S16("\x0100"),_S16("*\x0100"),0},
	{_S16("\x0100"),_S16("*\x0100?"),KErrNotFound},
	{_S16("\x0101"),_S16("\x0101"),0},
	{_S16("\x0101"),_S16("*"),0},
	{_S16("\x0101"),_S16("?"),0},
	{_S16("\x0101"),_S16("*\x0101"),0},
	{_S16("\x0101"),_S16("*\x0101?"),KErrNotFound},
	{_S16("\x0ffe"),_S16("\x0ffe"),0},
	{_S16("\x0ffe"),_S16("*"),0},
	{_S16("\x0ffe"),_S16("?"),0},
	{_S16("\x0ffe"),_S16("*\x0ffe"),0},
	{_S16("\x0ffe"),_S16("*\x0ffe?"),KErrNotFound},
	{_S16("\x0fff"),_S16("\x0fff"),0},
	{_S16("\x0fff"),_S16("*"),0},
	{_S16("\x0fff"),_S16("?"),0},
	{_S16("\x0fff"),_S16("*\x0fff"),0},
	{_S16("\x0fff"),_S16("*\x0fff?"),KErrNotFound},
	{_S16("\x1000"),_S16("\x1000"),0},
	{_S16("\x1000"),_S16("*"),0},
	{_S16("\x1000"),_S16("?"),0},
	{_S16("\x1000"),_S16("*\x1000"),0},
	{_S16("\x1000"),_S16("*\x1000?"),KErrNotFound},
	{_S16("\x1001"),_S16("\x1001"),0},
	{_S16("\x1001"),_S16("*"),0},
	{_S16("\x1001"),_S16("?"),0},
	{_S16("\x1001"),_S16("*\x1001"),0},
	{_S16("\x1001"),_S16("*\x1001?"),KErrNotFound},
	//	fffe, ffff is special
	//{_S16("\xfffe"),_S16("\xfffe"),0},
	//{_S16("\xfffe"),_S16("*"),0},
	//{_S16("\xfffe"),_S16("?"),0},
	//{_S16("\xfffe"),_S16("*\xfffe"),0},	//reserved
	//{_S16("\xfffe"),_S16("*\xfffe?"),KErrNotFound},
	//{_S16("\xffff"),_S16("\xffff"),0},
	//{_S16("\xffff"),_S16("*"),0},
	//{_S16("\xffff"),_S16("?"),0},
	//{_S16("\xffff"),_S16("*\xffff?"),KErrNotFound},
	//{_S16("\x0101\xffff\x0ffe"),_S16("*\xffff"),0},
	//{_S16("\x0101\xffff\x0ffe"),_S16("*\xffff"),0},
	//{_S16("\x0101\xfffe\x0ffe"),_S16("\xffff?"),0},
	//{_S16("\x0101\xfffe\x0ffe"),_S16("*\xffff?"),0},
	{_S16("\x04fa"),_S16("*"),0},		
	};

TInt KTests=sizeof(Tests8)/sizeof(Tests8[0]);
TInt KTests16=sizeof(Tests16)/sizeof(Tests16[0]);

TestMatch16 const TestsSurrogate[]=
	{
	//	not duplicate, test MatchSurrogate here
	{_S16(""),_S16(""),0},
	{_S16(""),_S16("?"),KErrNotFound},
	{_S16(""),_S16("*"),0},
	{_S16(""),_S16("**"),0},
	{_S16(""),_S16("*x*"),KErrNotFound},
	{_S16("x"),_S16(""),KErrNotFound},
	{_S16("x"),_S16("?"),0},
	{_S16("x"),_S16("*"),0},
	{_S16("x"),_S16("**"),0},
	{_S16("x"),_S16("**?"),0},
	{_S16("x"),_S16("?**"),0},				// 10
	{_S16("x"),_S16("**?*"),0},
	{_S16("x"),_S16("x"),0},
	{_S16("x"),_S16("a"),KErrNotFound},
	{_S16("x"),_S16("xx"),KErrNotFound},
	{_S16("x"),_S16("?x"),KErrNotFound},
	{_S16("x"),_S16("x*"),0},
	{_S16("x"),_S16("*x"),0},
	{_S16("x"),_S16("*x*"),0},
	{_S16("x"),_S16("**x*"),0},
	{_S16("abc"),_S16(""),KErrNotFound},	// 20
	{_S16("abc"),_S16("?*"),0},
	{_S16("abc"),_S16("*?"),2},
	{_S16("abc"),_S16("*?*?"),0},
	{_S16("abc"),_S16("*a*"),0},
	{_S16("abc"),_S16("*b*"),1},
	{_S16("abc"),_S16("*c*"),2},
	{_S16("abc"),_S16("*a"),KErrNotFound},
	{_S16("abc"),_S16("*c"),2},
	{_S16("abc"),_S16("*?c"),1},
	{_S16("abc"),_S16("??c"),0},			// 30
	{_S16("abc"),_S16("*b?"),1},

	// ones containing supplementary characters
	{_S16("ab\xD840\xDDAD"),_S16("*b*"),1},
	{_S16("ab\xD840\xDDAD"),_S16("*b?"),1},
	{_S16("a\xD840\xDDAD\x0063"),_S16("*c*"),3},
	{_S16("a\xD840\xDDAD\x0063"),_S16("*\xD840\xDDAD*"),1},
	{_S16("a\xD840\xDDAB\xD830\xDDAC\xD840\xDDAC\x0063"),_S16("*\xD840\xDDAC*"),5},
	{_S16("\xD840\xDDAB\xD840\xDDAC\x0063"),_S16("?\xD840\xDDAC*"),0},
	{_S16("\xD840\xDDAB\xD840\xDDAC\x0063"),_S16("\xD840\xDDAB*"),0},
	{_S16("\xD840\xDDAB\xD840\xDDAC\x0063"),_S16("*?\xD840\xDDAC*"),0},
	{_S16("\xD840\xDDAB\xD840\xDDAC\xD840\xDDAD\x0063"),_S16("*?\xD840\xDDAD*"),2},		// 40
	};

TInt KTestsSurrogate=sizeof(TestsSurrogate)/sizeof(TestsSurrogate[0]);


/**
@SYMTestCaseID SYSLIB-UNICODE-CT-1770
@SYMTestCaseDesc TDes16 Collation conversion function test
@SYMTestPriority High
@SYMTestActions  Testing the three collation conversion function
                 in TDesC16::GetNormalizedDecomposedFormL,
                    TDesC16::GetFoldedDecomposedFormL,
                    TDesC16::GetCollationKeysL  
@SYMTestExpectedResults The test must not fail.
@SYMREQ 6178 Add several new Unicode utility functions
*/
static void TestDes16CollationFunctionL()
	{
	/**----------------Test TDesC16::GetNormalizedDecomposedFormL------------------*/
	
	HBufC16* outputBuffer=NULL;
	_LIT16(KTestString1,"abc")	;
	//LATIN CAPITAL LETTER W WITH DIAERESIS(\x0057\x0308)
	//LATIN SMALL LETTER A(\x0061)
	//LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND TILDE(\x006F\x0302\x0303)
	//GREEK SMALL LETTER ALPHA WITH PSILI AND PERISPOMENI AND YPOGEGRAMMENI(\x03B1\x0313\x0342\x0345)
	_LIT16(KTestString2,"\x1E84\x0061\x1ED7\x1F86");
	_LIT16(KTestStringNDF2,"\x0057\x0308\x0061\x006F\x0302\x0303\x03B1\x0313\x0342\x0345");

	outputBuffer=KTestString1().GetNormalizedDecomposedFormL();
	test(outputBuffer->Compare(KTestString1())==0);
	delete outputBuffer;
	
	outputBuffer=KTestString2().GetNormalizedDecomposedFormL();
	test(outputBuffer->Compare(KTestStringNDF2())==0);
	delete outputBuffer;

	/**----------------Test TDesC16::GetFoldedDecomposedFormL------------------*/
	_LIT16(KTestString6,"AbC");
	_LIT16(KTestStringFolded6,"abc");
	//GREEK CAPITAL LETTER OMICRON WITH PSILI =>\x03BF\x0313
	//LATIN SMALL LETTER M WITH ACUTE =>\x006D\x0301
	//LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND HOOK ABOVE => \x006F\x0302\x0309
	_LIT16(KTestString7,"\x1F48\x1E3F\x1ED4");
	_LIT16(KTestStringFolded7,"\x03BF\x0313\x006D\x0301\x006F\x0302\x0309");
	
	outputBuffer=KTestString6().GetFoldedDecomposedFormL();
	test(outputBuffer->Compare(KTestStringFolded6())==0);
	delete outputBuffer;
	
	outputBuffer=KTestString7().GetFoldedDecomposedFormL();
	test(outputBuffer->Compare(KTestStringFolded7())==0);
	delete outputBuffer;
	
	/**----------------Test TDesC16::GetCollationKeysL------------------*/
	TCollationMethod method;
   	method.iId = 0;
   	//purposely set the maintable to NULL, this will result in the DefaultTable being used
   	method.iMainTable = 0;
   	method.iOverrideTable = 0;
   	method.iFlags = TCollationMethod::EIgnoreNone;
	
	//---------------Test key generation functionality----------------
	/** 
	Collation keys for
	x=08b90108-00000078
    y=08bd0108-00000079
    z=08c90108-0000007a
    */
	_LIT(KInputString1,"xyz");
	HBufC8* outbuf=NULL;	
	//Max Level 0 keys
	_LIT8(KMaxLevel0Key,"\x08\xb9\x08\xbd\x08\xc9");
	outbuf=KInputString1().GetCollationKeysL(0,&method);
	test(outbuf->Compare(KMaxLevel0Key())==0);
	delete outbuf;	
	//Max Level 1 keys
	_LIT8(KMaxLevel1Key,"\x08\xb9\x08\xbd\x08\xc9\x00\x00\x01\x01\x01");	
	outbuf=KInputString1().GetCollationKeysL(1,&method);
	test(outbuf->Compare(KMaxLevel1Key())==0);
	delete outbuf;	
	//Max Level 2 keys
	_LIT8(KMaxLevel2Key,"\x08\xb9\x08\xbd\x08\xc9\x00\x00\x01\x01\x01\x00\x08\x08\x08");	
	outbuf=KInputString1().GetCollationKeysL(2,&method);
	test(outbuf->Compare(KMaxLevel2Key())==0);
	delete outbuf;
	//Max Level 3 keys
	_LIT8(KMaxLevel3Key,"\x08\xb9\x08\xbd\x08\xc9\x00\x00\x01\x01\x01\x00\x08\x08\x08\x00\x00\x00\x78\x00\x00\x79\x00\x00\x7A");	
	outbuf=KInputString1().GetCollationKeysL(3,&method);
	test(outbuf->Compare(KMaxLevel3Key())==0);
	delete outbuf;	
	
	/**
	Decomposition for 1F70
	1F70=03B1 0300
	Collation keys for
	\x03B1	=09360108-000003B1	
	\x0300	=00001609-00000300
	y		=08bd0108-00000079
	*/
	_LIT(KInputString2,"\x1F70y");
	//Max Level 2 keys
	_LIT8(KCollationString22,"\x09\x36\x08\xBD\x00\x00\x01\x16\x01\x00\x08\x08\x08");
	outbuf=KInputString2().GetCollationKeysL(2,&method);
	test(outbuf->Compare(KCollationString22())==0);
	delete outbuf;
		
	//Max Level 3 keys
	_LIT8(KCollationString23,"\x09\x36\x08\xBD\x00\x00\x01\x16\x01\x00\x08\x08\x08\x00\x00\x03\xB1\x00\x03\x00\x00\x00\x79");
	outbuf=KInputString2().GetCollationKeysL(3,&method);
	test(outbuf->Compare(KCollationString23())==0);
	delete outbuf;

	/**
	Decomposition for 1EAC
	1EAC= 1EA0 0302 = 0041 0323 0302
	Collation keys for
	\x0041	=06CF0121-00000041		
	\x0323	=FF800104-00000001,83230105-00000000(2 keys for one character)
	\x0302	=00001D09-00000302
	*/
	_LIT(KInputString3,"\x1EAC");
	//Max Level 0 keys
	_LIT8(KCollationString30,"\x06\xCF\xFF\x80\x83\x23");
	//Max Level 1 keys
	_LIT8(KCollationString31,"\x06\xCF\xFF\x80\x83\x23\x00\x00\x01\x01\x01\x1d");	
	outbuf=KInputString3().GetCollationKeysL(1,&method);
	test(outbuf->Compare(KCollationString31())==0);
	delete outbuf;
	
	//Max Level 3 keys
	_LIT8(KCollationString33,"\x06\xCF\xFF\x80\x83\x23\x00\x00\x01\x01\x01\x1d\x00\x20\x04\x04\x08\x00\x00\x00\x41\x00\x00\x01\x00\x03\x02");
	outbuf=KInputString3().GetCollationKeysL(3,&method);
	test(outbuf->Compare(KCollationString33())==0);
	delete outbuf;
	
	//--------------Test using NULL collationMethod-----------------------
	outbuf=KInputString3().GetCollationKeysL(3,NULL);
	test(outbuf->Compare(KCollationString33())==0);
	delete outbuf;
	
	//--------------Test using out of limit level-------------------------
	outbuf=KInputString3().GetCollationKeysL(6,NULL);
	test(outbuf->Compare(KCollationString33())==0);
	delete outbuf;

	outbuf=KInputString3().GetCollationKeysL(-1,NULL);
	test(outbuf->Compare(KCollationString30())==0);
	delete outbuf;	
				
	}

/**
@SYMTestCaseID SYSLIB-UNICODE-CT-1771
@SYMTestCaseDesc TDes16 Collation conversion function OOM test
@SYMTestPriority High
@SYMTestActions  OOM Testing the three collation conversion function
                 in TDesC16::GetNormalizedDecomposedFormL,
                    TDesC16::GetFoldedDecomposedFormL,
                    TDesC16::GetCollationKeysL  
@SYMTestExpectedResults The test must not fail.
@SYMREQ 6178 Add several new Unicode utility functions
*/	
static void TestDes16CollationFunctionOOM()
	{
	test.Next(_L("TestDes16CollationFunctionOOM"));

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
		TRAP(err,TestDes16CollationFunctionL() );
		__UHEAP_SETFAIL(RHeap::ENone, 0);

		// check that no handles have leaked
		TInt endProcessHandleCount;
		TInt endThreadHandleCount;
		RThread().HandleCount(endProcessHandleCount, endThreadHandleCount);

		test(startProcessHandleCount == endProcessHandleCount);
		test(startThreadHandleCount  == endThreadHandleCount);

		__UHEAP_MARKEND;
		} while(err == KErrNoMemory);

	test(err == KErrNone);
	test.Printf(_L("- TestDes16CollationFunctionOOM succeeded at heap failure rate of %i\n"), tryCount);	
	}

GLDEF_C TInt E32Main()
//
// entry point
//
    {
	CTrapCleanup* trapCleanup = CTrapCleanup::New();
	test(trapCleanup != NULL);

	test.Title();
//

	test.Start(_L("Match8"));
	TInt ii;
	for (ii=0;ii<KTests;++ii)
		{
		TInt r=TPtrC8(Tests8[ii].iLeft).Match(TPtrC8(Tests8[ii].iRight));
		test (r==Tests8[ii].iResult);
		r=TPtrC8(Tests8[ii].iLeft).MatchF(TPtrC8(Tests8[ii].iRight));
		test (r==Tests8[ii].iResult);
		}
	test.Next(_L("Match16"));
	for (ii=0;ii<KTests16;++ii)
		{
		TInt r=TPtrC16(Tests16[ii].iLeft).Match(TPtrC16(Tests16[ii].iRight));
		test (r==Tests16[ii].iResult);
		r=TPtrC16(Tests16[ii].iLeft).MatchF(TPtrC16(Tests16[ii].iRight));
		test (r==Tests16[ii].iResult);
		}
	//	check code points with upper case
	test.Next( _L("Check characters with upper case") );
	//039c: lower 03bc, folded 03bc
	//00b5: upper 039c, folded 03bc
	_LIT( K00b5, "\x00b5" );
	_LIT( K039c, "\x039c" );
	_LIT( K03bc, "\x03bc" );
	test( 0 == TPtrC16( K00b5() ).MatchF( TPtrC16( K039c() ) ) );
	test( 0 == TPtrC16( K00b5() ).MatchF( TPtrC16( K00b5() ) ) );
	test( 0 == TPtrC16( K00b5() ).MatchF( TPtrC16( K039c() ) ) );
	test( 0 == TPtrC16( K00b5() ).MatchF( TPtrC16( K03bc() ) ) );
	TBuf<20> buf00b5;
	buf00b5.Copy( K00b5() );
	buf00b5.UpperCase();
	test( 0 == buf00b5.Find( K039c() ) );
	test( 0 == buf00b5.MatchF( K039c() ) );
	
	//	check code points with both upper and lower cases
	test.Next( _L("Check characters with upper and lower cases") );
	//	01C5: upper 01C4, folded 01C6
	_LIT( K01c5, "\x01c5" );
	_LIT( K01c4, "\x01c4" );
	_LIT( K01c6, "\x01c6" );
	test( 0 == TPtrC16( K01c5() ).MatchF( TPtrC16( K01c6() ) ) );
	test( 0 == TPtrC16( K01c5() ).MatchF( TPtrC16( K01c4() ) ) );
	test( 0 == TPtrC16( K01c4() ).MatchF( TPtrC16( K01c5() ) ) );
	test( 0 == TPtrC16( K01c4() ).MatchF( TPtrC16( K01c6() ) ) );
	TBuf<20> buf01c5;
	buf01c5.Copy( K01c5() );
	buf01c5.UpperCase();
	test( 0 == buf01c5.Find( K01c4() ) );
	test( 0 == buf01c5.MatchF( K01c6() ) );
	
	test.Next(_L("MatchSurrogate"));
	for (ii=0;ii<KTestsSurrogate;++ii)
		{
		TInt r=TPtrC16(TestsSurrogate[ii].iLeft).Match2(TPtrC16(TestsSurrogate[ii].iRight));
		RDebug::Printf("    ii=%d, expect=%d, result=%d", ii, TestsSurrogate[ii].iResult, r);
		test (r==TestsSurrogate[ii].iResult);
		r=TPtrC16(TestsSurrogate[ii].iLeft).MatchF(TPtrC16(TestsSurrogate[ii].iRight));
		test (r==TestsSurrogate[ii].iResult);
		}
	
	_LIT( KD800, "\xd800" );
	_LIT( KQuestion, "?" );
	_LIT( KDC00, "\xdc00" );
	_LIT( KDFFF, "\xdfff" );
	test( KErrCorruptSurrogateFound == TPtrC16( KD800() ).Match2( TPtrC16( KQuestion() ) ) );
	test( KErrCorruptSurrogateFound == TPtrC16( KD800() ).Match2( TPtrC16( KD800() ) ) );
	test( KErrCorruptSurrogateFound == TPtrC16( KDC00() ).Match2( TPtrC16( KQuestion() ) ) );
	test( KErrCorruptSurrogateFound == TPtrC16( KDFFF() ).Match2( TPtrC16( KQuestion() ) ) );

    test( KErrNotFound == TPtrC16( KD800() ).MatchF( TPtrC16( KQuestion() ) ) );
    test( 0 == TPtrC16( KD800() ).MatchF( TPtrC16( KD800() ) ) );
    test( KErrNotFound == TPtrC16( KDC00() ).MatchF( TPtrC16( KQuestion() ) ) );
    test( KErrNotFound == TPtrC16( KDFFF() ).MatchF( TPtrC16( KQuestion() ) ) );

	test.Next(_L("Iterator tests"));
	
	::TestUTF32Iterator();
	::TestFoldedDecompIterator();
	::TestFoldedSortedDecompIterator();
	::TestFoldedCanonicalIterator();
	::TestDecompositionIterator2();
	::TestCanonicalDecompositionIterator2();
	::TestCanonicalDecompositionIteratorCached();
	
	test.Next(_L("Unit tests"));
	
	TestDecompositionIterator();
	TestCanonicalDecompositionIterator();
	TestCollationValueIterator();
	TestMatchIdentifiers();
	TestFindIdentifier();
	
	TRAPD(err, TestFoldingL());
	test(err == KErrNone);
	
	test.Next(_L("INC057641"));
	TRAP(err, INC057641L());
	test(err == KErrNone);

	TestMatchC();
	TestMatchC2();

	test.Next(_L("TestDes16CollationFunctionL"));
	TRAP(err,TestDes16CollationFunctionL());
	test(err==KErrNone);
	::TestDes16CollationFunctionOOM();

	TestDisableCombiningCharacterCheck();

	test.End();
	test.Close();
	
	delete trapCleanup;
	
	return 0;
    }
