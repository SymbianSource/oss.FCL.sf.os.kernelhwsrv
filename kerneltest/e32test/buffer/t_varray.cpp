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
// e32test\buffer\t_varray.cpp
// Overview:
// Test variable record length array classes.
// API Information:
// CArrayVarFlat, CArrayVarSeg.
// Details:
// - Create an array of variable length text using a flat dynamic & segmented
// buffer and verify that:
// - number of elements held in the array is 0.
// - array is compressed and reset as expected.
// - the elements of the array are sorted as expected.
// - insertion of a text character into the array at specified position 
// and length of the array is as expected.
// - return value is 0 when available element is searched using sequential 
// search technique within the array.
// - removal of first element from the array is successful.
// - number of elements held in the array is 1 after appending a single 
// element at the end of empty array.
// - insertion of a single element with specified key is successful.
// - the element is found when searched using binary search technique
// - KErrAlreadyExists is returned if an element is inserted with the same 
// key already exists within the array.
// - Create an array of variable length text character implemented using a flat 
// dynamic & segmented buffer.
// - append some strings onto the end of the array, check the contents and 
// number of elements held in the array are as expected.
// - insert some strings and verify that the change in the content of array 
// and number of elements held in the array are as expected.
// - remove a single, multiple elements from the array and verify that the 
// Delete method is as expected.
// - Create an array of variable length text character contained within a flat 
// dynamic & segmented buffer.
// - append some strings of specified length onto the end of the array, compress 
// the array and verify that the number of elements held in the array is as specified.
// - insert a string at specified location, check the contents and reset the array.
// - append some strings at the end, insert some strings at specified position, 
// compress the array and verify that content, number of strings in the array 
// are as expected.
// - test that the number of elements and contents of the array are sorted as expected.
// - verify the correct position of the element and return value is zero when 
// an element is found using binary, sequential search technique and nonzero 
// if not present in the array.
// - insert some strings at the specified location and check that correct position
// is returned when searched using binary search technique.
// - Create an array of variable length integer contained within a flat dynamic &
// segmented buffer.
// - insert some elements with same key which is already present within the array 
// and check that KErrAlreadyExists is returned. 
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

const TInt KTestGranularity=0x02;

LOCAL_D RTest test(_L("T_VARRAY"));

LOCAL_C void testAllMethods(CArrayVar<TText>& aVar)
    {
	test.Next(_L("Test all methods"));
	test(aVar.Count()==0);
	aVar.Compress();
	test(TRUE);
	aVar.Reset();
	test(TRUE);
	TKeyArrayVar kk(sizeof(TText),ECmpNormal,0);
	test(TRUE);
	aVar.Sort(kk);
	test(TRUE);
	const TText* aa=_S("a");
	aVar.InsertL(0,*aa,sizeof(TText));
	test(aVar.Length(0)==sizeof(TText));
	test(TRUE);
	TInt pp;
	test(aVar.Find(*aa,kk,pp)==0);
	test(pp==0);
	aVar.Delete(0);
	aVar.AppendL(*aa,1);
	test(aVar.Count()==1);
	aVar.InsertIsqAllowDuplicatesL(*aa,0,kk);
	test(TRUE);
	test(aVar.FindIsq(*aa,kk,pp)==0);
	test(pp==0);
	TRAPD(r,aVar.InsertIsqL(*aa,0,kk));
	test(r==KErrAlreadyExists);
    }

LOCAL_C void test1(CArrayVar<TText>& aVar)
//
	{
	test.Next(_L("AppendL and InsertL chars"));
	aVar.AppendL(*_S("abcd"),5*sizeof(TText)); // abcd
	TBuf<0x10> des1(&aVar[0]);
	test(des1==_L("abcd"));
	test(aVar.Count()==1);
	aVar.AppendL(*_S("wxyz"),5*sizeof(TText)); // abcd wxyz
	des1=&aVar[1];
	test(des1==_L("wxyz"));
	test(aVar.Count()==2);
	aVar.InsertL(1,*_S("ef"),3*sizeof(TText)); // abcd ef wxyz
	des1=&aVar[1];
	test(des1==_L("ef"));
	test(aVar.Count()==3);	
	aVar.AppendL(*_S("z"),2*sizeof(TText)); // abcd ef wxyz z
	des1=&aVar[3];
	test(des1==_L("z"));
	aVar.InsertL(0,*_S("y"),2*sizeof(TText)); // y abcd ef wxyz z
	des1=&aVar[0];
	test(des1==_L("y"));
	test(aVar.Length(0)==2*sizeof(TText));
	test(aVar.Length(1)==5*sizeof(TText));
	test(aVar.Length(2)==3*sizeof(TText));
	test(aVar.Length(3)==5*sizeof(TText));
	test(aVar.Length(4)==2*sizeof(TText));
	des1=&aVar[1];
	test(des1==_L("abcd"));
	test(aVar.Count()==5);
//
	test.Next(_L("Delete chars"));
	aVar.Delete(3,1); // y abcd ef z
	des1=&aVar[2];
	test(des1==_L("ef"));
	des1=&aVar[1];
	test(des1==_L("abcd"));
	des1=&aVar[3];
	test(des1==_L("z"));
	aVar.Delete(1,2); // y z
	des1=&aVar[0];
	test(des1==_L("y"));
	des1=&aVar[1];
	test(des1==_L("z"));
	test(aVar.Count()==2);
	}

LOCAL_C void test2(CArrayVar<TText>& aVar)
//
	{
	test.Next(_L("Reset and Compress"));
	TBuf<0x10> des1(_L("abcde"));
	TBuf<0x10> des2(_L("fgh"));
	TBuf<0x10> des3(_L("wxyz"));
	aVar.AppendL(*(TText*)des1.Ptr(),des1.Size());
	aVar.AppendL(*(TText*)des2.Ptr(),des2.Size());
	aVar.Compress();
	test(aVar.Count()==2);
	TPtrC des4((TText*)&aVar[0],des1.Length());
	test(des1==des4);
	TPtrC des5((TText*)&aVar[1],des2.Length());
	test(des2==des5);
	aVar.InsertL(1,*(TText*)des3.Ptr(),des3.Size());
	test(aVar.Count()==3);	
	TPtrC des6((TText*)&aVar[0],des1.Length());
	test(des1==des6);
	TPtrC des7((TText*)&aVar[2],des2.Length());
	test(des2==des7);
	TPtrC des8((TText*)&aVar[1],des3.Length());
	test(des3==des8);
	aVar.Reset();

	TBuf<0x10> buf1=_L("abcdef");
	TBuf<0x10> buf2=_L("wxyz");
	TBuf<0x10> buf3=_L("lmnop");
	TBuf<0x10> buf4=_L("aa");
	aVar.AppendL(*buf1.Ptr(),buf1.Size());
	aVar.InsertL(0,*buf2.Ptr(),buf2.Size());
	aVar.AppendL(*buf3.Ptr(),buf3.Size());
	aVar.InsertL(1,*buf4.Ptr(),buf4.Size());
	aVar.Compress();
	TPtrC rd1((TText*)&aVar[2],buf1.Length());
	test(buf1==rd1);
	TPtrC rd2((TText*)&aVar[0],buf2.Length());
	test(buf2==rd2);
	TPtrC rd3((TText*)&aVar[3],buf3.Length());
	test(buf3==rd3);
	TPtrC rd4((TText*)&aVar[1],buf4.Length());
	test(buf4==rd4);
	test(aVar.Count()==4);
	
	TKeyArrayVar kk(0,ECmpNormal,3);		// Compare 3 characters
	TKeyArrayVar kk1(0,ECmpNormal,2);	// Compare 2 characters
	test.Next(_L("Sort"));
	aVar.Sort(kk);
	TPtrC rd5((TText*)&aVar[1],buf1.Length());
	test(buf1==rd5);
	TPtrC rd6((TText*)&aVar[3],buf2.Length());
	test(buf2==rd6);
	TPtrC rd7((TText*)&aVar[2],buf3.Length());
	test(buf3==rd7);
	TPtrC rd8((TText*)&aVar[0],buf4.Length());
	test(buf4==rd8);
	test(aVar.Count()==4);	

	test.Next(_L("Find and FindIsq"));
	TBuf<0x10> buf5=_L("ffff");
	test(aVar.InsertIsqL(*(TText*)buf5.Ptr(),buf5.Size(),kk)==2);
	TRAPD(r,aVar.InsertIsqL(*(TText*)buf5.Ptr(),buf5.Size(),kk))
	test(r==KErrAlreadyExists);
	test(aVar.InsertIsqAllowDuplicatesL(*(TText*)buf5.Ptr(),buf5.Size(),kk)==3);
	TInt aPos;
	test(aVar.Find(*_S("abc"),kk,aPos)==0);	// Second parameter 'aLength' is unused. 
	test(aPos==1);
	test(aVar.Find(*_S("aa"),kk1,aPos)==0);
	test(aPos==0);
	test(aVar.Find(*_S("wxyz"),kk,aPos)==0);
	test(aPos==5);
	test(aVar.Find(*_S("fgh"),kk,aPos)!=0);		// Returns !=0 if string not found.
	test(aPos==6);								// Not present in list, aPos set to last position
	test(aVar.Find(*_S("ffff"),kk,aPos)==0);	
	test(aPos==2);
	test(aVar.Find(*_S("lmn"),kk,aPos)==0);
	test(aPos==4);
	test(aVar.FindIsq(*_S("abc"),kk,aPos)==0);
	test(aPos==1);
	test(aVar.FindIsq(*_S("aa"),kk1,aPos)==0);
	test(aPos==0);
	test(aVar.FindIsq(*_S("wxyz"),kk,aPos)==0);
	test(aPos==5);
	test(aVar.FindIsq(*_S("fgh"),kk,aPos)!=0);	// Returns result of last test
	test(aPos==4);		  // Not present in list, aPos set to last position tested
	TBuf<0x10> buf7=_L("fgh");
	test(aVar.InsertIsqL(*(TText*)buf7.Ptr(),buf7.Size(),kk)==4);
	test(aVar.FindIsq(*_S("fgh"),kk,aPos)==0);	// Returns result of last test
	test(aPos==4);
									
	test(aVar.FindIsq(*_S("ffff"),kk,aPos)==0);
	test(aPos==3);
	test(aVar.FindIsq(*_S("lmn"),kk,aPos)==0);
	test(aPos==5); 
	}

LOCAL_C void test3(CArrayVar<TInt>& aVar)
	{

	test.Next(_L("InsertIsqL"));
	TKeyArrayVar kk(0,ECmpTInt);

	TInt pos=0;
	TInt mod=47;
	TInt inc=23;

	TInt i=0;	
	FOREVER
		{
		TInt ret;
		if (i&1)
			TRAP(ret,aVar.InsertIsqL(i,sizeof(TInt),kk))
		else
			{
			TRAP(ret,pos=aVar.InsertIsqL(i,sizeof(TInt),kk))
			if (ret==KErrNone)
				test(aVar[pos]==i);
			}
		if (ret==KErrAlreadyExists)
			break;
		i=(i+inc)%mod;
		}
	for(i=0;i<mod;i++)
		{
		pos=(-1);
		test(aVar.FindIsq(i,kk,pos)==0);
		test(pos==i);
		TRAPD(r,aVar.InsertIsqL(i,sizeof(TInt),kk))
		test(r==KErrAlreadyExists);
		}
	}

GLDEF_C TInt E32Main()
//
// Test the variable record length array classes.
//
    {

	test.Title();
	__UHEAP_MARK;
	test.Start(_L("class CArrayFixFlat"));
//
	CArrayVarFlat<TText>* pVarFlat=new CArrayVarFlat<TText>(KTestGranularity);
	if (pVarFlat==NULL)
		test.Panic(_L("Allocating array"));
    testAllMethods(*pVarFlat);
    delete pVarFlat;
//
	CArrayVarFlat<TText>* pVarFlatChar=new CArrayVarFlat<TText>(KTestGranularity);
	test1(*pVarFlatChar);
	delete pVarFlatChar; 
//		
	CArrayVarFlat<TText>* pVarFlatArr=new CArrayVarFlat<TText>(KTestGranularity);
	test2(*pVarFlatArr);
	delete pVarFlatArr;
//
	CArrayVarFlat<TInt>* pVarFlatInt=new CArrayVarFlat<TInt>(KTestGranularity);
	test3(*pVarFlatInt);
	delete pVarFlatInt;
//	
	test.Next(_L("class CArrayVarSeg"));
	CArrayVarSeg<TText>* pVarSeg=new CArrayVarSeg<TText>(KTestGranularity);
	if (pVarSeg==NULL)
		test.Panic(_L("Allocating array"));
	testAllMethods(*pVarSeg);
	delete pVarSeg;
//
	CArrayVarSeg<TText>* pVarSegChar=new CArrayVarSeg<TText>(KTestGranularity);
	test1(*pVarSegChar);
	delete pVarSegChar;
//		
	CArrayVarSeg<TText>* pVarSegArr=new CArrayVarSeg<TText>(KTestGranularity);
	test2(*pVarSegArr);
	delete pVarSegArr;
//
	CArrayVarSeg<TInt>* pVarSegInt=new CArrayVarSeg<TInt>(KTestGranularity);
	test3(*pVarSegInt);
	delete pVarSegInt;
//
	test.End();
	__UHEAP_MARKEND;
	return(0);
    }

