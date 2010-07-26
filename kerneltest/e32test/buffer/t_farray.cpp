// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\buffer\t_farray.cpp
// Overview:
// Test the functionality of CArrayFixFlat,  CArrayPtrFlat,  CArrayFixSeg classes.
// API Information:
// CArrayFixFlat, CArrayPtrFlat, CArrayFixSeg.
// Details:
// - Create an array of fixed length buffer descriptor objects contained within a flat
// dynamic and segmented buffer and verify that
// - number of elements held in the array is 0.
// - length of an element is as specified.
// - array is compressed and reset as expected.
// - the elements of the array are sorted as expected.
// - insertion of a text into the array at specified position and filling a blank space
// at the beginning is as expected.
// - return value is 0 when available element is searched within the array.
// - removal of first element from the array is successful.
// - number of elements held in the array is 1 after appending a single element onto 
// the end of the array.
// - the position of specified element is found successfully
// - resetting the array is as expected
// - End and Back methods are as expected.
// - Create an array of fixed length text character objects contained within a flat dynamic 
// and segmented buffer.
// - append a text onto the end of the array, check the contents and number of elements 
// held in the array are as expected.
// - insert a text and verify that the change in the content of array and number of 
// elements held in the array are as expected.
// - remove a single character and multiple characters from the array and verify that
// the Delete method is as expected. Compress the array.
// - Create an array of fixed length text character objects contained within a flat dynamic
// and segmented buffer.
// - append strings of specified length onto the end of the array and verify that the 
// number of elements held in the array is as expected.
// - insert strings at specified location and check that the contents are as expected.
// - reset the array, append a string, compress the array and verify that content is as 
// expected.
// - sort the array and verify that content is as expected.
// - verify the correct position of the element and return value is zero when an element
// is found using binary and sequential search technique and nonzero if not present in 
// the array.
// - Create an array of fixed length text character objects contained within a flat dynamic 
// and segmented buffer.
// - Insert some elements into the array at specified positions determined by key of 
// type TInt and verify that KErrAlreadyExists is returned if an element with the 
// same key already exists within the array.
// - Create an array of pointers to objects implemented using a flat dynamic buffer, insert one 
// element into the array at the specified position and destroy the object whose pointer form 
// the element of the array, before resetting the array.
// - Create and delete an array of CBase objects contained within a flat dynamic buffer.
// - Test whether the heap has been corrupted by all the tests.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

class MyCBase : public CBase
	{
	};

const TInt KTestGranularity=0x02;

LOCAL_D RTest test(_L("T_FARRAY"));

template <class T,TInt S>
class TArr
	{
public:
	TArr() {}
	TInt Count() const {return S;}
	T& operator[](TInt anIndex) {return iArr[anIndex];}
	const T& operator[](TInt anIndex) const {return iArr[anIndex];}
private:
	T iArr[S];
	};

LOCAL_C void testFix(CArrayFix<TBuf<0x10> >& aFix)
//
// Test all methods
//
	{
	test.Next(_L("Test all methods"));
	test(aFix.Count()==0);
	test(aFix.Length()==sizeof(TBuf<0x10>));
	aFix.Compress();
	test(TRUE);
	aFix.Reset();
	test(TRUE);
	TKeyArrayFix kk(0,ECmpNormal,0x10);
	test(TRUE);
	aFix.Sort(kk);
	test(TRUE);
	TBuf<0x10> aa(_L("aaaaa"));
	aFix.InsertL(0,aa);
	test(TRUE);
	aFix[0].Fill(' ');
	test(TRUE);
	TBuf<0x10> z(aFix[0]);
    z.Length();
	test(TRUE);
	aFix[0].Fill('a');
	test(TRUE);
	TInt pp;
	test(aFix.Find(aa,kk,pp)==0);
	test(pp==0);
	aFix.Delete(0);
	TBuf<0x10> bb(_L("bbbbb"));
	aFix.AppendL(bb);
	test(aFix.Count()==1);
	test(aFix.InsertIsqAllowDuplicatesL(aa,kk)==0);
	test(aFix.InsertIsqAllowDuplicatesL(bb,kk)==2);
	test(aFix.FindIsq(aa,kk,pp)==0);
	test(pp==0);
	aFix.Reset();
	for(TInt index=0;index<KTestGranularity*7/2;index++)
		aFix.AppendL(aa);
	const TBuf<0x10> *end=NULL;
	const TBuf<0x10> *ptr=NULL;
	for(TInt index2=0;index2<KTestGranularity*7/2;index2++)
		{
		if (end==ptr)
			{
			end=aFix.End(index2);
			ptr=&aFix[index2];
			TInt seglen=end-ptr;
			test(seglen==KTestGranularity || seglen==(aFix.Count()-index2));
			}
		test(&aFix[index2]==ptr++);
		}
	const TBuf<0x10> *bak=NULL;
	ptr=NULL;
	for(TInt index3=KTestGranularity*7/2;index3>0;index3--)
		{
		if (bak==ptr)
			{
			bak=aFix.Back(index3);
			ptr=&aFix[index3-1]+1;
			TInt seglen=ptr-bak;
			test(seglen==KTestGranularity || seglen==index3 || seglen==index3%KTestGranularity);
			}
		test(&aFix[index3-1]==--ptr);
		}
	
	//Test ExpandL
	//Expand array in slot 1
	TBuf16<0x10> exp;
	exp=_L("abc AbC");
	aFix.InsertL(0,exp);
	aFix.InsertL(1,exp);
	aFix.InsertL(2,exp);
	exp=aFix.ExpandL(1);
	test(aFix[0]==_L("abc AbC"));
	test(aFix[1]==_L(""));
	test(aFix[2]==_L("abc AbC"));
	test(aFix[3]==_L("abc AbC"));
	
	//Test ResizeL and InsertReplL
	//Resize the array to containing 20 records,
	//copying a record into any new slots.
	TBuf<0x10> res(_L("bbbbb"));
	aFix.Reset();
	aFix.ResizeL(20,res);
	for(TInt i=0;i<20;i++)
	    {
        test(aFix[1]==_L("bbbbb"));
	    }
	}

LOCAL_C void test1(CArrayFix<TText>& aFix)
//
	{
	test.Next(_L("AppendL and InsertL single chars"));
	aFix.AppendL(_S("abcd"),4);
	test(aFix[0]=='a');
	test(aFix[1]=='b');
	test(aFix[3]=='d');
	test(aFix.Count()==4);
	aFix.InsertL(2,_S("ef"),2);
	test(aFix[1]=='b');
	test(aFix[2]=='e');
	test(aFix[4]=='c');
	test(aFix.Count()==6);	
	aFix.AppendL(TText('z'));
	test(aFix[6]=='z');
	aFix.InsertL(0,TText('y'));
	test(aFix[0]=='y');
	test(aFix[1]=='a');
	test(aFix.Count()==8);
	test.Next(_L("Delete single chars"));
	aFix.Delete(3);
	test(aFix[2]=='b');
	test(aFix[3]=='f');
	test(aFix[4]=='c');
	aFix.Delete(1,2);
	test(aFix[0]=='y');
	test(aFix[1]=='f');
	test(aFix[2]=='c');
	test(aFix.Count()==5);
	aFix.Compress();
	}

LOCAL_C void test2(CArrayFix<TArr<TText,4> >& aFix)
//
	{
	test(aFix.Length()==sizeof(TArr<TText,4>));
	test.Next(_L("AppendL and insert strings of length 4"));
	TPtrC des1=_L("abcd");
	TPtrC des2=_L("efgh");
	aFix.AppendL(*(const TArr<TText,4>*)des1.Ptr());
	aFix.AppendL(*(const TArr<TText,4>*)des2.Ptr());
	test(aFix.Count()==2);
	TPtrC des3(&aFix[0][0],4);
	TPtrC des4(&aFix[1][0],4);
	test(des3==_L("abcd"));
	test(des4==_L("efgh"));
	aFix.InsertL(1,*(const TArr<TText,4>*)_S("ijkl"));
	test(aFix.Count()==3);	
	TPtrC des5(&aFix[2][0],4);
	test(des3==_L("abcd"));
	test(des4==_L("ijkl"));
	test(des5==_L("efgh"));

	test.Next(_L("Reset and Compress"));
	aFix.Reset();
	TBuf<0x10> buf1=_L("abcdefgh");
	aFix.AppendL((const TArr<TText,4>*)buf1.Ptr(),2);
	aFix.Compress();
	TPtrC des6(&aFix[0][0],4);
	test(des6==_L("abcd"));
	TPtrC des7(&aFix[1][0],4);
	test(des7==_L("efgh"));
	buf1=_L("ghighhxy");
	aFix.InsertL(1,(const TArr<TText,4>*)buf1.Ptr(),2);
	aFix.Compress();
	TPtrC des8(&aFix[0][0],4);
	test(des8==_L("abcd"));
	TPtrC des9(&aFix[1][0],4);
	test(des9==_L("ghig"));
	TPtrC des10(&aFix[2][0],4);
	test(des10==_L("hhxy"));
	TPtrC des11(&aFix[3][0],4);
	test(des11==_L("efgh"));

	test.Next(_L("Sort strings"));
	TKeyArrayFix kk(0,ECmpNormal,0x04);
	aFix.Sort(kk);
	TPtrC des12(&aFix[0][0],4);
	test(des12==_L("abcd"));
	TPtrC des13(&aFix[1][0],4);
	test(des13==_L("efgh"));
	TPtrC des14(&aFix[2][0],4);
	test(des14==_L("ghig"));
	TPtrC des15(&aFix[3][0],4);
	test(des15==_L("hhxy"));
	
	test.Next(_L("Find and FindIsq"));
	aFix.Compress();
	test(aFix.InsertIsqL(*(const TArr<TText,4>*)_S("ffff"),kk)==2);
	aFix.Compress();
	test(aFix.InsertIsqAllowDuplicatesL(*(const TArr<TText,4>*)_S("ffff"),kk)==3);
	aFix.Compress();
	TRAPD(r,aFix.InsertIsqL(*(const TArr<TText,4>*)_S("ffff"),kk))
	test(r==KErrAlreadyExists);
	TInt aPos=0;
	test(aFix.Find(*(const TArr<TText,4>*)_S("xxxx"),kk,aPos)==1);
	test(aPos==6);
	test(aFix.Find(*(const TArr<TText,4>*)_S("abcd"),kk,aPos)==0);
	test(aPos==0);
	test(aFix.Find(*(const TArr<TText,4>*)_S("ghig"),kk,aPos)==0);
	test(aPos==4);
	test(aFix.Find(*(const TArr<TText,4>*)_S("ffff"),kk,aPos)==0);
	test(aPos==2);
	test(aFix.Find(*(const TArr<TText,4>*)_S("hhxy"),kk,aPos)==0);
	test(aPos==5);
	test(aFix.FindIsq(*(const TArr<TText,4>*)_S("bbbb"),kk,aPos)!=0);
	test(aPos==1);
	test(aFix.FindIsq(*(const TArr<TText,4>*)_S("abcd"),kk,aPos)==0);
	test(aPos==0);
	test(aFix.FindIsq(*(const TArr<TText,4>*)_S("ghig"),kk,aPos)==0);
	test(aPos==4);
	test(aFix.FindIsq(*(const TArr<TText,4>*)_S("ffff"),kk,aPos)==0);
	test(aPos==2);
	test(aFix.InsertIsqL(*(const TArr<TText,4>*)_S("fghz"),kk)==4);
	test(aFix.FindIsq(*(const TArr<TText,4>*)_S("fghz"),kk,aPos)==0);
	test(aPos==4);
	test(aFix.FindIsq(*(const TArr<TText,4>*)_S("hhxy"),kk,aPos)==0);
	test(aPos==6);
	}

LOCAL_C void test3(CArrayFix<TInt>& aFix)
	{

	test.Next(_L("InsertIsqL"));
	TKeyArrayFix kk(0,ECmpTInt);

	TInt pos=0;
	TInt mod=47;
	TInt inc=23;
	TInt i=0;

	FOREVER
		{
		TInt ret;
		if (i&1)
			TRAP(ret,aFix.InsertIsqL(i,kk))
		else
			{
			TRAP(ret,pos=aFix.InsertIsqL(i,kk))
			if (ret==KErrNone)
				test(aFix[pos]==i);
			}
		if (ret==KErrAlreadyExists)
			break;
		i=(i+inc)%mod;
		}

	for(i=0;i<mod;i++)
		{
		test(aFix.FindIsq(i,kk,pos)==0);
		test(pos==i);
		TRAPD(r,aFix.InsertIsqL(i,kk))
		test(r==KErrAlreadyExists);
		}
	}
	
GLDEF_C TInt E32Main()
//
// Test the Array classes.
//
    {

	test.Title();
	__UHEAP_MARK;
	test.Start(_L("class CArrayFixFlat"));
	CArrayFixFlat<TBuf<0x10> >* pFixFlat=new CArrayFixFlat<TBuf<0x10> >(KTestGranularity);
	if (pFixFlat==NULL)
		test.Panic(_L("Allocating array"));
	testFix(*pFixFlat);
	delete pFixFlat;

	CArrayFixFlat<TText>* pFixFlatChar=new CArrayFixFlat<TText>(KTestGranularity);
	test1(*pFixFlatChar);
	delete pFixFlatChar; 
		
	CArrayFixFlat<TArr<TText,4> >* pFixFlatArr=new CArrayFixFlat<TArr<TText,4> >(KTestGranularity);
	test2(*pFixFlatArr);
	delete pFixFlatArr;

	CArrayFixFlat<TInt>* pFixFlatInt=new CArrayFixFlat<TInt>(KTestGranularity);
	test3(*pFixFlatInt);
	delete pFixFlatInt;
	
	CArrayFixFlat<TUid>* pFixFlatTUid=new CArrayFixFlat<TUid>(KTestGranularity);
	if (pFixFlatTUid==NULL)
		{
	    test.Panic(_L("Allocating array of TUid"));
		}
	delete pFixFlatTUid; 

	test.Next(_L("class CArrayPtrFlat of CBase"));
	
	CArrayPtrFlat<MyCBase>* pPtrFlatCBase=new CArrayPtrFlat<MyCBase>(KTestGranularity); 
	if (pPtrFlatCBase==NULL)
		test.Panic(_L("Allocating array of CBase*"));
	MyCBase* c1 = new MyCBase();
	pPtrFlatCBase->InsertL(0,&c1,1);
	pPtrFlatCBase->ResetAndDestroy();
//	test(pFixFlatCBase->iBase==0);
	pPtrFlatCBase->ResetAndDestroy();
	delete pPtrFlatCBase;

	test.Next(_L("class CArrayFixFlat of CBase"));
	
	CArrayFixFlat<MyCBase>* pFixFlatCBase=new CArrayFixFlat<MyCBase>(KTestGranularity); 
	if (pFixFlatCBase==NULL)
		test.Panic(_L("Allocating array of CBase"));
	delete pFixFlatCBase;

	test.Next(_L("class CArrayFixSeg"));
	CArrayFixSeg<TBuf<0x10> >* pFixSeg=new CArrayFixSeg<TBuf<0x10> >(KTestGranularity);
	if (pFixSeg==NULL)
		test.Panic(_L("Allocating array"));
	testFix(*pFixSeg);
	delete pFixSeg;

	CArrayFixSeg<TText>* pFixSegChar=new CArrayFixSeg<TText>(KTestGranularity);
	test1(*pFixSegChar);
	delete pFixSegChar; 
		
	CArrayFixSeg<TArr<TText,4> >* pFixSegArr=new CArrayFixSeg<TArr<TText,4> >(KTestGranularity);
	test2(*pFixSegArr);
	delete pFixSegArr;

	CArrayFixSeg<TInt>* pFixSegInt=new CArrayFixSeg<TInt>(KTestGranularity);
	test3(*pFixSegInt);
	delete pFixSegInt;

	test.End();
	__UHEAP_MARKEND;
	return(0);
    }

