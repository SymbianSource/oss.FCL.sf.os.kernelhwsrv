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
// e32test\buffer\t_parray.cpp
// Overview:
// Test the variable record length array classes.
// Test only covers the Flat type implementation.
// API Information:
// CArrayPakFlat.
// Details:
// - Create an array of variable length text character objects packed into a flat buffer.
// - check the number of elements held in the array is 0.
// - test that Compress and Reset methods are as expected.
// - sort the array and check that no error is returned.
// - insert an element and check the length is as expected.
// - search for the inserted element and check it is found successfully.
// - remove all the elements, append an element and verify that number of elements
// held in the array is 1.
// - insert an element into the array, search for that element using sequential,
// binary search technique and verify that it is found at the expected position.
// - insert another element with the same key and check that KErrAlreadyExists is 
// returned.
// - Create an array of variable length text character objects packed into a flat buffer.
// - append some strings at the end, insert some strings at the specified location and
// verify that the length of each string and number of strings held in the array are 
// as expected.
// - delete some strings and check the remaining strings in the array are as expected.
// - Create an array of variable length text character objects packed into a flat buffer.
// - append some strings at the end and insert some stings at specified location.
// - compress the array and verify strings and the number of strings held in the 
// array are as expected.
// - reset the array and verify the number of elements held in the array is 0.
// - sort the array and check that array is sorted as expected.
// - test that KErrAlreadyExists is returned if another element with the same key 
// type is inserted.
// - search for strings using sequential, binary search and verify that 0 is returned 
// if found else nonzero.
// - delete some text from the array and check the results are as expected.
// - Create an array of variable length integer objects packed into a flat buffer.
// - Insert some elements with same key which already exists within the array and 
// check that KErrAlreadyExists is returned.
// - Test whether the heap has been corrupted by all the tests.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32svr.h>
#include <e32ver.h>

const TInt KTestGranularity=0x10;

LOCAL_D RTest test(_L("T_PARRAY"));

LOCAL_C void testAllMethods(CArrayPak<TText>& aPakVar)
    {
	test.Next(_L("Test all methods"));
	test(aPakVar.Count()==0);
	aPakVar.Compress();
	test(TRUE);
	aPakVar.Reset();
	test(TRUE);
	TKeyArrayPak kk(sizeof(TText),ECmpNormal,0);
	TKeyArrayVar vv(sizeof(TText),ECmpNormal,0);
	TKeyArrayPak hh(sizeof(TText),ECmpNormal);
	test(TRUE);
	TRAPD(res,aPakVar.SortL(vv));
	test(res==KErrNone);
	TRAPD(err,aPakVar.SortL(hh));
	test(err==KErrNone);
	const TText* aa=_S("a");
	aPakVar.InsertL(0,*aa,sizeof(TText));
	TBuf<0x10> des1(1);
	des1[0]=aPakVar[0];
	test(des1==_L("a"));
	test(aPakVar.Length(0)==sizeof(TText));
	test(TRUE);
	TInt pp;
	test(aPakVar.Find(*aa,kk,pp)==0);
	test(pp==0);
	aPakVar.Delete(0);
	aPakVar.AppendL(*aa,1);
	test(aPakVar.Count()==1);
	aPakVar.InsertIsqAllowDuplicatesL(*aa,0,kk);
	test(TRUE);
	test(aPakVar.FindIsq(*aa,kk,pp)==0);
	test(pp==0);
	TRAPD(r,aPakVar.InsertIsqL(*aa,0,kk));
	test(r==KErrAlreadyExists);
	}

LOCAL_C void test1(CArrayPak<TText>& aPakVar)
//
	{
	test.Next(_L("AppendL and InsertL chars"));
	aPakVar.AppendL(*_S("abcd"),5*sizeof(TText)); // abcd
	TBuf<0x10> des1(&aPakVar[0]);
	test(des1==_L("abcd"));
	test(aPakVar.Count()==1);
	aPakVar.AppendL(*_S("wxyz"),5*sizeof(TText)); // abcd wxyz
	des1=&aPakVar[1];
	test(des1==_L("wxyz"));
	test(aPakVar.Count()==2);
	aPakVar.InsertL(1,*_S("ef"),3*sizeof(TText)); // abcd ef wxyz
	des1=&aPakVar[1];
	test(des1==_L("ef"));
	test(aPakVar.Count()==3);	
	aPakVar.AppendL(*_S("z"),2*sizeof(TText)); // abcd ef wxyz z
	des1=&aPakVar[3];
	test(des1==_L("z"));
	aPakVar.InsertL(0,*_S("y"),2*sizeof(TText)); // y abcd ef wxyz z
	des1=&aPakVar[0];
	test(des1==_L("y"));
	test(aPakVar.Length(0)==2*sizeof(TText));
	test(aPakVar.Length(1)==5*sizeof(TText));
	test(aPakVar.Length(2)==3*sizeof(TText));
	test(aPakVar.Length(3)==5*sizeof(TText));
	test(aPakVar.Length(4)==2*sizeof(TText));
	des1=&aPakVar[1];
	test(des1==_L("abcd"));
	test(aPakVar.Count()==5);
//
	test.Next(_L("Delete chars"));
	aPakVar.Delete(3,1); // y abcd ef z
	des1=&aPakVar[2];
	test(des1==_L("ef"));
	des1=&aPakVar[1];
	test(des1==_L("abcd"));
	des1=&aPakVar[3];
	test(des1==_L("z"));
	aPakVar.Delete(1,2); // y z
	des1=&aPakVar[0];
	test(des1==_L("y"));
	des1=&aPakVar[1];
	test(des1==_L("z"));
	test(aPakVar.Count()==2);
	}

LOCAL_C void test2(CArrayPak<TText>& aPakVar)
//
	{
	test.Next(_L("Reset and Compress"));
	TBuf<0x10> des1(_L("abcde"));
	TBuf<0x10> des2(_L("fgh"));
	TBuf<0x10> des3(_L("wxyz"));
	aPakVar.AppendL(*(TText*)des1.Ptr(),des1.Size());
	aPakVar.AppendL(*(TText*)des2.Ptr(),des2.Size());
	aPakVar.Compress();
	test(aPakVar.Count()==2);
	TPtrC des4((TText*)&aPakVar[0],des1.Length());
	test(des1==des4);
	TPtrC des5((TText*)&aPakVar[1],des2.Length());
	test(des2==des5);
	aPakVar.InsertL(1,*(TText*)des3.Ptr(),des3.Size());
	test(aPakVar.Count()==3);	
	TPtrC des6((TText*)&aPakVar[0],des1.Length());
	test(des1==des6);
	TPtrC des7((TText*)&aPakVar[2],des2.Length());
	test(des2==des7);
	TPtrC des8((TText*)&aPakVar[1],des3.Length());
	test(des3==des8);
	aPakVar.Reset();
	// So nothing in this array
	test(aPakVar.Count()==0);
	TKeyArrayPak kk(0,ECmpNormal,3);		// Compare 3 characters
	TKeyArrayPak kk1(0,ECmpNormal,2);	// Compare 2 characters
	TKeyArrayVar vv(0,ECmpNormal,3);		// Compare 3 characters
	TBuf<0x10> buf1=_L("abcdef");
	TBuf<0x10> buf2=_L("wxyz");
	TBuf<0x10> buf3=_L("lmnop");
	TBuf<0x10> buf4=_L("aa");
	aPakVar.AppendL(*buf1.Ptr(),buf1.Size());
	aPakVar.InsertL(0,*buf2.Ptr(),buf2.Size());
	aPakVar.AppendL(*buf3.Ptr(),buf3.Size());
	aPakVar.InsertL(1,*buf4.Ptr(),buf4.Size());
	aPakVar.Compress();
	TPtrC rd1((TText*)&aPakVar[2],buf1.Length());
	test(buf1==rd1);
	TPtrC rd2((TText*)&aPakVar[0],buf2.Length());
	test(buf2==rd2);
	TPtrC rd3((TText*)&aPakVar[3],buf3.Length());
	test(buf3==rd3);
	TPtrC rd4((TText*)&aPakVar[1],buf4.Length());
	test(buf4==rd4);
	test(aPakVar.Count()==4);

	test.Next(_L("Sort"));
	TRAPD(res,aPakVar.SortL(vv));
	test(res==KErrNone);
	/**/
	TPtrC rd5((TText*)&aPakVar[1],buf1.Length());
	test(buf1==rd5);
	TPtrC rd6((TText*)&aPakVar[3],buf2.Length());
	test(buf2==rd6);
	TPtrC rd7((TText*)&aPakVar[2],buf3.Length());
	test(buf3==rd7);
	TPtrC rd8((TText*)&aPakVar[0],buf4.Length());
	test(buf4==rd8);
	test(aPakVar.Count()==4);	
	/**/
	test.Next(_L("Find and FindIsq"));
	TBuf<0x10> buf5=_L("ffff");
	test(aPakVar.InsertIsqL(*(TText*)buf5.Ptr(),buf5.Size(),kk)==2);
	TRAPD(r,aPakVar.InsertIsqL(*(TText*)buf5.Ptr(),buf5.Size(),kk))
	test(r==KErrAlreadyExists);
	test(aPakVar.InsertIsqAllowDuplicatesL(*(TText*)buf5.Ptr(),buf5.Size(),kk)==3);
	TInt aPos;
	test(aPakVar.Find(*_S("abc"),kk,aPos)==0);	// Second parameter 'aLength' is unused. 
	test(aPos==1);
	test(aPakVar.Find(*_S("aa"),kk1,aPos)==0);
	test(aPos==0);
	test(aPakVar.Find(*_S("wxyz"),kk,aPos)==0);
	test(aPos==5);
	test(aPakVar.Find(*_S("fgh"),kk,aPos)!=0);		// Returns non zero if string not found.
	test(aPos==6);								// Not present in list, aPos set to last position
	test(aPakVar.Find(*_S("ffff"),kk,aPos)==0);	
	test(aPos==2);
	test(aPakVar.Find(*_S("lmn"),kk,aPos)==0);
	test(aPos==4);			  //15
	test(aPakVar.FindIsq(*_S("abc"),kk,aPos)==0);
	test(aPos==1);
	test(aPakVar.FindIsq(*_S("aa"),kk1,aPos)==0);
	test(aPos==0);
	test(aPakVar.FindIsq(*_S("wxyz"),kk,aPos)==0);
	test(aPakVar.FindIsq(*_S("fgh"),kk,aPos)!=0);	// 22 Returns result of last test
	test(aPos==4);		// Not present in list, aPos set to last position tested
	//This test shows problem with BinarySearch
	TBuf<0x10> buf7=_L("fghij");
	test(aPakVar.InsertIsqL(*(TText*)buf7.Ptr(),buf7.Size(),kk)==4);
	test(aPakVar.FindIsq(*_S("fgh"),kk,aPos)==0);	// Returns result of last test
	test(aPos==4);
	test(aPakVar.FindIsq(*_S("ffff"),kk,aPos)==0);
	test(aPos==3);
	test(aPakVar.FindIsq(*_S("lmn"),kk,aPos)==0);
	test(aPos==5); 
	aPakVar.Delete(4,1);
	test(aPakVar.Find(*_S("wxyz"),kk,aPos)==0);
	test(aPos==5);
	}

LOCAL_C void test3(CArrayPak<TInt>& aPak)

	{
	test.Next(_L("InsertIsqL"));
	TKeyArrayPak kk(0,ECmpTInt);

	TInt pos=0;
	TInt mod=47;
	TInt inc=23;

	TInt i=0;	
	FOREVER
		{
		TInt ret;
		if (i&1)
			TRAP(ret,aPak.InsertIsqL(i,sizeof(TInt),kk))
		else
			{
			TRAP(ret,pos=aPak.InsertIsqL(i,sizeof(TInt),kk))
			if (ret==KErrNone)
				test(aPak[pos]==i);
			}
		if (ret==KErrAlreadyExists)
			break;
		i=(i+inc)%mod;
		}
	for(i=0;i<mod;i++)
		{
		pos=(-1);
		test(aPak.FindIsq(i,kk,pos)==0);
		test(pos==i);
		TRAPD(r,aPak.InsertIsqL(i,sizeof(TInt),kk))
		test(r==KErrAlreadyExists);
		}
	}

GLDEF_C TInt E32Main()
//
// Test the variable record length array classes.
// Initially test only covers the Flat type implementation.
//
    {
	test.Title();
	__UHEAP_MARK;
	test.Start(_L("class CArrayPakFlat"));
//
    CArrayPakFlat<TText>* pPakFlat=new CArrayPakFlat<TText>(KTestGranularity);
	testAllMethods(*pPakFlat);
    delete pPakFlat;
//
	CArrayPakFlat<TText>* pPakFlatChar=new CArrayPakFlat<TText>(KTestGranularity);
	test1(*pPakFlatChar);
	delete pPakFlatChar; 
//
 	CArrayPakFlat<TText>* pPakFlatArr=new CArrayPakFlat<TText>(KTestGranularity);
	test2(*pPakFlatArr);
	delete pPakFlatArr;
//
	CArrayPakFlat<TInt>* pPakFlatInt=new CArrayPakFlat<TInt>(KTestGranularity);
	test3(*pPakFlatInt);
	delete pPakFlatInt;
//
	test.End();
	__UHEAP_MARKEND;
	return(0);
    }

