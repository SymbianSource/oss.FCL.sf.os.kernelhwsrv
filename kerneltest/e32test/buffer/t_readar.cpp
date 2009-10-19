// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\buffer\t_readar.cpp
// Overview:
// Test the CArrayFixFlat, CArrayPakFlat, CArrayVarFlat classes.
// API Information:
// CArrayFixFlat, CArrayPakFlat, CArrayVarFlat.
// Details:
// - Create an array of fixed length objects contained within a flat 
// dynamic buffer, append some elements onto the end of the array, 
// sort the array into key sequence, check the number of elements 
// in the array is as expected and read all of elements. Check 
// whether the heap has been corrupted.
// - Create an array of variable length objects implemented using a 
// flat dynamic buffer, append some elements onto the end of the 
// array, sort the array into key sequence, check the number of elements 
// in the array is as expected and read all of elements. Check whether 
// the heap has been corrupted.
// - Create an array of variable length objects packed into a flat buffer, 
// append some elements onto the end of the array, sort the array into 
// key sequence and check the number of elements held in the array is 
// as expected. Read all array elements. Check whether the heap has been 
// corrupted.
// - Check whether the heap has been corrupted by any of the tests.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

const TInt KMaxStrings=3;

LOCAL_D RTest test(_L("T_READAR"));
LOCAL_D const TPtrC s1(_L("ZZZZ"));
LOCAL_D const TPtrC s2(_L("AAAA"));
LOCAL_D const TPtrC s3(_L("MMMM"));
LOCAL_D const TPtrC* str[KMaxStrings] = {&s1,&s2,&s3};
LOCAL_D const TPtrC* strSorted[KMaxStrings] = {&s2,&s3,&s1};

LOCAL_C void testReadAny(const TArray<TBufC<0x20> > anArray)
//
// Test with fixed length arrays.
//
	{

	test(anArray.Count()==KMaxStrings);
	for (TInt i=0;i<KMaxStrings;i++)
		test(anArray[i]==(*strSorted[i]));
	}

LOCAL_C void testFixL()
//
// Test with fixed length arrays.
//
	{

	__UHEAP_MARK;
//
	test.Start(_L("Creating Fix array"));
	CArrayFixFlat<TBufC<0x20> >* pFix=new(ELeave) CArrayFixFlat<TBufC<0x20> >(1);
	for (TInt i=0;i<KMaxStrings;i++)
		{
		TBufC<0x20> b=(*str[i]);
		pFix->AppendL(b);
		}
//
	test.Next(_L("Sorting Fix array"));
    TKeyArrayFix array(0,ECmpNormal);
	pFix->Sort(array);
//
	test.Next(_L("Reading Fix array"));
	testReadAny(pFix->Array());
//
	test.Next(_L("Destroying Fix array"));
	delete pFix;
	__UHEAP_MARKEND;
//
	test.End();
	}

LOCAL_C void testVarL()
//
// Test with variable length arrays.
//
	{

	__UHEAP_MARK;
//
	test.Start(_L("Creating Var array"));
	CArrayVarFlat<TBufC<0x20> >* pVar=new(ELeave) CArrayVarFlat<TBufC<0x20> >(1);
	for (TInt i=0;i<KMaxStrings;i++)
		{
		TBufC<0x20> b=(*str[i]);
		pVar->AppendL(b,b.Size()+sizeof(TUint));
		}
//
	test.Next(_L("Sorting Var array"));
    TKeyArrayVar array(0,ECmpNormal);
	pVar->Sort(array);
//
	test.Next(_L("Reading Var array"));
	testReadAny(pVar->Array());
//
	test.Next(_L("Destroying Var array"));
	delete pVar;
	__UHEAP_MARKEND;
//
	test.End();
	}

LOCAL_C void testPakL()
//
// Test with variable length packed arrays.
//
	{

	__UHEAP_MARK;
//
	test.Start(_L("Creating Pak array"));
	CArrayPakFlat<TBufC<0x20> >* pPak=new(ELeave) CArrayPakFlat<TBufC<0x20> >(1);
	for (TInt i=0;i<KMaxStrings;i++)
		{
		TBufC<0x20> b=(*str[i]);
		pPak->AppendL(b,b.Size()+sizeof(TUint));
		}
//
	test.Next(_L("Sorting Pak array"));
    TKeyArrayVar array(0,ECmpNormal);
	pPak->SortL(array);
//
	test.Next(_L("Reading Pak array"));
	testReadAny(pPak->Array());
//
	test.Next(_L("Destroying Pak array"));
	delete pPak;
	__UHEAP_MARKEND;
//
	test.End();
	}

GLDEF_C TInt E32Main()
//
// Test the Array classes.
//
    {

	test.Title();
	__UHEAP_MARK;
//
	test.Start(_L("Testing Fix arrays"));
	TRAPD(r,testFixL());
	test(r==KErrNone);
//
	test.Next(_L("Testing Var arrays"));
	TRAP(r,testVarL());
	test(r==KErrNone);
//
	test.Next(_L("Testing Pak arrays"));
	TRAP(r,testPakL());
	test(r==KErrNone);
//
	__UHEAP_MARKEND;
	test.End();
	return(0);
    }

