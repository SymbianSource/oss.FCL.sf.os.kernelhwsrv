// Copyright (c) 1994-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\buffer\t_regn.cpp
// Overview:
// Test fixed and variable clipping regions.
// API Information:
// TRegion, TRegionFix, RRegion .
// Details:
// - Construct some expandable clipping regions, add some rectangles, check the region 
// matches the rectangles, clear the region, add some rectangles to the region and 
// check the region matches the rectangles.
// - Copy one region to another, using the Copy method and the copy constructor,
// and check the region matches the rectangles.
// - Create a some fixed clipping regions, add some rectangles, check the region
// matches the rectangles, clear the region, add some rectangles, and check the
// region matches the rectangles.
// - Copy one fixed region to another, using the Copy method and the copy constructor,
// and check the region matches the rectangles.
// - Test TRegionFix creation and error handling using Clear, Count, AddRect, CheckError
// and Tidy methods
// - Test adding rectangles, via AddRect, to an RRegion object. Verify the results 
// via the Count, BoundingRect and IsEmpty methods.
// - Test subtracting rectangles, via SubRect, from an RRegion object. Verify the 
// results via the Count, BoundingRect and SubRegion methods.
// - Test subtracting regions, via AddRect and SubRegion, from an RRegion object. 
// Verify the results via the Count, BoundingRect, Clear, Copy, and SubRect methods.
// - Test the RRegion Tidy method. Verify the results via the Count, AddRect, 
// BoundingRect and Clear methods.
// - Test the RRegion CheckSpare method. Verify the results via the AddRect, Tidy,
// Clear and SubRect methods.
// - Test the RRegion Offset method. Verify the results via the AddRect, Move,
// Clear, IsEmpty and RectangleList methods.
// - Test the RRegion Intersection and Intersect methods. Verify the results via 
// the AddRect, Count, IsEmpty and RectangleList methods.
// - Test the RRegion Union method. Verify the results via the AddRect, Count,
// Copy, Offset and BoundingRect methods.
// - Test the RRegion ClipRect method. Verify the results via the AddRect, Count,
// and BoundingRect methods.
// - Test the RRegion and TRgionFix Contains methods. Verify the results via the 
// AddRect method.
// - Test the RRegion ForceError and CheckError methods. Verify the results via the 
// AddRect, Copy, Count, SubRect, Clear and BoundingRect methods.
// - Test the RRegion  and RRegionBuf sort method.
// - Construct some regions with pre-allocated buffer (RRegionBuf), add some rectangles, 
// get a pointer to the array of rectangles defining this region and check the 
// rectangles are as expected.
// - Test IsContainedBy method to check whether some rects cover the region used
// as reference or are outside or overlapping
// - Test Destroy method for a heap allocated object
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_REGN"));

class TestRRegion
	{
public:
	TestRRegion(TInt tlx, TInt tly, TInt brx, TInt bry);
	void TestSet();
	void TestRegionFix();
	void TestAddRect();
	void TestSubRect();
	void TestSubRegion();
	void TestTidy();
	void TestSpare();
	void TestOffset();
	void TestIntersection();
	void TestUnion();
	void TestClipRect();
	void TestContains();
	void TestIntersects();
	void TestErrors();
	void doTestSort(RRegion &aRegion);
	void TestSort();
	void doTestRegionBuf(RRegion &aRegion);
	void TestRegionBuf();
	void TestIsContainedBy();
	void TestDestroy();
private:
	void DoTestSet(TRegion** rgn,TInt rgnArraySize);
	void CheckRectRegion(const TRegion& region,const TRect& rect);
private:
	TRect rect[4];
	TRect bounds;
	TRect xrect;
	};

// Region test code
TestRRegion::TestRRegion(TInt tlx, TInt tly, TInt brx, TInt bry)
	{
	rect[0]=TRect( tlx, tly, brx, bry);
	rect[1]=TRect(-brx,-bry,-tlx,-tly);
	rect[2]=TRect( tlx,-bry, brx,-tly);
	rect[3]=TRect(-brx, tly,-tlx, bry);
	bounds=TRect(-brx,-bry,brx,bry);
	xrect=TRect(-(tlx/2+brx/2),-(tly/2+bry/2),tlx/2+brx/2,tly/2+bry/2);
	}

void TestRRegion::CheckRectRegion(const TRegion& region,const TRect& rect)
// Check the region matches the rectangle
	{
	const TRect* rlist;

	if (rect.IsEmpty())
		test(region.Count()==0);
	else
		{
		test(region.Count()==1);
		rlist=region.RectangleList();
		test(rlist[0]==rect);
		test(region[0]==rect);
		}
	}

void TestRRegion::DoTestSet(TRegion** rgn,TInt rgnArraySize)
	{
	TInt index; 
	for(index=0;index<rgnArraySize;index++)
		rgn[index]->AddRect(rect[index]);
	for(index=0;index<rgnArraySize;index++)
		CheckRectRegion(*rgn[index],rect[index]);
	for(index=0;index<rgnArraySize;index++)
		{
		rgn[index]->Clear();
		rgn[index]->AddRect(rect[index]);
		}
	for(index=0;index<rgnArraySize;index++)
		CheckRectRegion(*rgn[index],rect[index]);
	}

void TestRRegion::TestSet()
	{
	TUint index; 

	RRegion xrgn(rect[0]);
	CheckRectRegion(xrgn,rect[0]);
	xrgn.Close();
//
	RRegion rgn[5];
	TRegion* prgn[5]={&rgn[0],&rgn[1],&rgn[2],&rgn[3],&rgn[4]};
	DoTestSet(&prgn[0],(sizeof(rgn)/sizeof(rgn[0])));
	for(index=0;index<(sizeof(rgn)/sizeof(rgn[0]));index++)
		{
		RRegion rgn1;
		rgn1.Copy(rgn[index]);
		CheckRectRegion(rgn1,rect[index]);
		RRegion rgn2(rgn[index]);
		CheckRectRegion(rgn2,rect[index]);
		rgn[index].Close();
		rgn1.Close();
		}
//
	TRegionFix<5> rgnf[5];
	TRegion* prgnf[5]={&rgnf[0],&rgnf[1],&rgnf[2],&rgnf[3],&rgnf[4]};
	DoTestSet(&prgnf[0],(sizeof(rgnf)/sizeof(rgnf[0])));
	for(index=0;index<(sizeof(rgn)/sizeof(rgn[0]));index++)
		{
		TRegionFix<5> rgn1;
		rgn1.Copy(rgnf[index]);
		CheckRectRegion(rgn1,rect[index]);
		TRegionFix<5> rgn2(rgnf[index]);
		CheckRectRegion(rgn2,rect[index]);
		}
	}

void TestRRegion::TestRegionFix()
//
// Test TRegionFix creation and error handling
//
	{
	TRegionFix<1> rgnf(rect[0]);
	CheckRectRegion(rgnf,rect[0]);
	rgnf.Clear();
	test(rgnf.Count()==0);
	rgnf.AddRect(TRect(0,0,2,2));
	test(rgnf.CheckError()==FALSE);
	rgnf.AddRect(TRect(2,2,4,4));	// Should cause error, rgnf can only hold 1 rectangle
	test(rgnf.CheckError()==TRUE && rgnf.Count()==0);
	rgnf.Clear();
	test(rgnf.CheckError()==FALSE && rgnf.Count()==0);
//
	TRegionFix<10> rgnf2;
	TInt index;
	for(index=0;index<10;index++)
		{
		rgnf2.AddRect(TRect(index*4,0,index*4+2,10));
		test(rgnf2.Count()==(index+1));
		TRegionFix<10> rgnf4(rgnf2);	// Test copy constructor
		TRegionFix<10> rgnf5;
		rgnf5=rgnf2;					// Test assignment
		test(rgnf4.Count()==(index+1));
		for(TInt index2=0;index2<=index;index2++)
			test(rgnf2[index2]==rgnf4[index2] && rgnf2[index2]==rgnf5[index2]);
		}
	rgnf2.AddRect(TRect(-10,-10,0,0));	// Should push it over the edge
	test(rgnf2.CheckError()==TRUE && rgnf2.Count()==0);
//
	TRegionFix<5> rgnf3;
	for(index=0;index<4;index++)
		{
		rgnf3.AddRect(TRect(index*4,index*4,index*4+8,index*4+8));
		if (index==3)
			test(rgnf3.CheckError()==TRUE);
		else
			{			
			rgnf3.Tidy();
			if (index>0)
				test(rgnf3.Count()==(index+2));
			}
		}
	}

void TestRRegion::TestAddRect()
	{
	RRegion rgn;
	TInt index,i;

	if (!rect[0].IsEmpty())
		{
		for(index=0;index<4;index++)
			{
			for(i=0;i<=index;i++)
				rgn.AddRect(rect[index]);
			test(rgn.Count()==(index+1));
			}
		test(rgn.BoundingRect()==bounds);
		if (!xrect.IsEmpty())
			{
			rgn.AddRect(xrect);
			TInt count = rgn.Count();
			test( (count > 4) && (count <= 9) );
			}
		}
	rgn.AddRect(bounds);
	test(rgn.Count()==1);
	rgn.Close();
	}

void TestRRegion::TestSubRect()
	{
	TRect rect1(-rect[0].iBr.iX,-rect[0].iBr.iY,rect[0].iBr.iX,rect[0].iBr.iY);
	RRegion rgn;
	RRegion subRgn;
	RRegion rgn2;
	TInt index;

	if (!rect[0].IsEmpty())
		{
		rgn.AddRect(rect1);
		for(index=0;index<4;index++)
			rgn.SubRect(rect[index],&subRgn);
		if (rect[0].iTl.iX==0)	// Special case region, all rects join in the middle
			{
			test(rgn.Count()==0);
			test(subRgn.Count()==4);
			}
		else
			{
			test(rgn.Count()==3);
			test(subRgn.Count()==4);
			test(rgn.BoundingRect()==subRgn.BoundingRect());
			rgn.SubRect(xrect);
			test(rgn.Count()==4);
			rgn2.Copy(rgn);
			subRgn.Clear();
			rgn.SubRect(rgn.BoundingRect(),&subRgn);
			test(rgn.Count()==0);
			rgn2.SubRegion(subRgn,&rgn);
			test(rgn2.Count()==0);
			subRgn.SubRegion(rgn);
			test(subRgn.Count()==0);
			}
		}
	rgn.Close();
	rgn2.Close();
	subRgn.Close();
	}

void TestRRegion::TestSubRegion()
	{
	TRect rect1(-rect[0].iBr.iX,-rect[0].iBr.iY,rect[0].iBr.iX,rect[0].iBr.iY);
	RRegion rgn,subRgn;
	RRegion rgn2;
	TInt index;

	if (!rect[0].IsEmpty())
		{
		rgn.AddRect(rect1);
		for(index=0;index<4;index++)
			rgn2.AddRect(rect[index]);
		rgn.SubRegion(rgn2,&subRgn);
		if (rect[0].iTl.iX==0)	// Special case region, all rects join in the middle
			{
			test(rgn.Count()==0);
			test(subRgn.Count()==4);
			}
		else
			{
			test(rgn.Count()==3);
			test(subRgn.Count()==4);
			test(rgn.BoundingRect()==subRgn.BoundingRect());
			rgn2.Clear();
			rgn2.AddRect(xrect);
			rgn.SubRegion(rgn2);
			test(rgn.Count()==4);
			rgn2.Copy(rgn);
			subRgn.Clear();
			rgn.SubRect(rgn.BoundingRect(),&subRgn);
			test(rgn.Count()==0);
			rgn2.SubRegion(subRgn,&rgn);
			test(rgn2.Count()==0);
			subRgn.SubRegion(rgn);
			test(subRgn.Count()==0);
			}
		}
	rgn.Close();
	rgn2.Close();
	subRgn.Close();
	}

void TestRRegion::TestTidy()
	{
	RRegion rgn;
	TInt loop;
	TRect const rlist[8]={	// 8 Rectangles that form a square with the centre rectangle missing
		TRect(10,10,20,20),
		TRect(20,10,30,20),
		TRect(30,10,40,20),

		TRect(10,20,20,30),
		TRect(30,20,40,30),

		TRect(10,30,20,40),
		TRect(20,30,30,40),
		TRect(30,30,40,40)};
	TRect const rect1(rlist[0].iTl.iX,rlist[0].iTl.iY,rlist[2].iBr.iX,rlist[2].iBr.iY);
	TRect const rect2(rlist[0].iTl.iX,rlist[0].iTl.iY,rlist[7].iBr.iX,rlist[7].iBr.iY);

// Add 3 adjoining rectangles and tidy them
	for(loop=0;loop<3;loop++)
		rgn.AddRect(rlist[loop]);
	test(rgn.Count()==3);
	rgn.Tidy();
	test(rgn.Count()==1);
	test(rgn[0]==rect1);
// Same again but add the adjoining rectangles in reverse order
	rgn.Clear();
	for(loop=2;loop>=0;loop--)
		rgn.AddRect(rlist[loop]);
	test(rgn.Count()==3);
	rgn.Tidy();
	test(rgn.Count()==1);
	test(rgn[0]==rect1);
// Now add 8 rectangles that should tidy down to 4
	rgn.Clear();
	for(loop=0;loop<8;loop++)
		rgn.AddRect(rlist[loop]);
	test(rgn.Count()==8);
	rgn.Tidy();
	test(rgn.Count()==4);
	test(rgn.BoundingRect()==rect2);
// ...and in reverse
	rgn.Clear();
	for(loop=7;loop>=0;loop--)
		rgn.AddRect(rlist[loop]);
	test(rgn.Count()==8);
	rgn.Tidy();
	test(rgn.Count()==4);
	test(rgn.BoundingRect()==rect2);
	rgn.Close();
	}

void TestRRegion::TestSpare()
	{
	TInt gran[5]={1,2,4,20,100};
	RRegion rgn[5]={gran[0],gran[1],gran[2],gran[3],gran[4]};
    TUint index;
	TInt loop,spare;
	TRect rect1(0,0,1,1);
	TRect rect;

	for(index=0;index<(sizeof(rgn)/sizeof(rgn[0]));index++)
		{
		test(rgn[index].CheckSpare()==0);
		rgn[index].AddRect(rect1);
		test(rgn[index].CheckSpare()==(gran[index]-1));
		rgn[index].Tidy();
		test(rgn[index].CheckSpare()<gran[index]);
		rgn[index].Clear();
		test(rgn[index].CheckSpare()==0);
		}
	for(index=0;index<(sizeof(rgn)/sizeof(rgn[0]));index++)
		{
		spare=0;
		for(loop=0;loop<30;loop++)
			{
			rect.iTl.iX=rect.iTl.iY=loop;
			rect.iBr.iX=rect.iBr.iY=loop+1;
			rgn[index].AddRect(rect);
			if (spare==0)
				spare=gran[index];
			spare--;
			test(rgn[index].CheckSpare()==spare);
			}
		do
			{
			loop--;
			rect.iTl.iX=rect.iTl.iY=loop;
			rect.iBr.iX=rect.iBr.iY=loop+1;
			rgn[index].SubRect(rect);
			spare++;
			test(rgn[index].CheckSpare()==spare);
			} while(loop>0);
		}
	for(index=0;index<(sizeof(rgn)/sizeof(rgn[0]));index++)
		rgn[index].Close();
	}

void TestRRegion::TestOffset()
	{
	RRegion rgn;
	const TRect* rlist;
	TRect r;
	TUint index;

	for(index=0;index<(sizeof(rect)/sizeof(rect[0]));index++)
		{
		rgn.Clear();
		rgn.AddRect(rect[index]);
		r=rect[index];
		r.Move(1,1);
		rgn.Offset(1,1);
		if (rect[index].IsEmpty())
			test(rgn.Count()==0);
		else
			{
			test(rgn.Count()==1);
			rlist=rgn.RectangleList();
			test(rlist[0]==r);
			}
		}
	rgn.Close();
	}

void TestRRegion::TestIntersection()
	{
	RRegion rgn1,rgn2,tmp_rgn;
	const TRect* rlist;
	TUint index;

	rgn1.AddRect(xrect);
	rgn2.Copy(rgn1);
	if (!rgn1.IsEmpty())
		{
		for(index=0;index<(sizeof(rect)/sizeof(rect[0]));index++)
			tmp_rgn.AddRect(rect[index]);
		rgn2.Intersection(rgn1,tmp_rgn);
		test(rgn2.Count()==4);
		rlist=rgn2.RectangleList();
		for(index=0;index<(TUint)rgn2.Count();index++)
			{
			test(rlist[index].iTl.iX==xrect.iTl.iX || rlist[index].iTl.iX==rect[0].iTl.iX);
			test(rlist[index].iTl.iY==xrect.iTl.iY || rlist[index].iTl.iY==rect[0].iTl.iY);
			test(rlist[index].iBr.iX==xrect.iBr.iX || rlist[index].iBr.iX==(-rect[0].iTl.iX));
			test(rlist[index].iBr.iY==xrect.iBr.iY || rlist[index].iBr.iY==(-rect[0].iTl.iY));
			}
		rgn1.Intersect(tmp_rgn);
		test(rgn1.Count()==4);
		rlist=rgn1.RectangleList();
		for(index=0;index<(TUint)rgn1.Count();index++)
			{
			test(rlist[index].iTl.iX==xrect.iTl.iX || rlist[index].iTl.iX==rect[0].iTl.iX);
			test(rlist[index].iTl.iY==xrect.iTl.iY || rlist[index].iTl.iY==rect[0].iTl.iY);
			test(rlist[index].iBr.iX==xrect.iBr.iX || rlist[index].iBr.iX==(-rect[0].iTl.iX));
			test(rlist[index].iBr.iY==xrect.iBr.iY || rlist[index].iBr.iY==(-rect[0].iTl.iY));
			}
		}
	rgn1.Close();
	rgn2.Close();
	tmp_rgn.Close();
	}

void TestRRegion::TestUnion()
	{
	RRegion rgn1,rgn2,rgn3,tmp_rgn;
	TRect bounds, rgn1Bounds;
	TUint index;

	// Test RRegion (always has a dynamic buffer).
	rgn1.AddRect(xrect);
	if (!rgn1.IsEmpty())
		{
		for(index=0;index<(sizeof(rect)/sizeof(rect[0]));index++)
			tmp_rgn.AddRect(rect[index]);
		test(tmp_rgn.Count()==4);
		rgn1.Union(tmp_rgn);
		test(rgn1.Count()==7);
		rgn1Bounds = rgn1.BoundingRect();
		rgn2.Copy(rgn1);
		rgn2.Offset(3,5);
		rgn3.Copy(rgn1);
		rgn3.Offset(5,7);
		rgn3.Union(rgn2);
		test(rgn3.Count()==17);
		bounds=rgn1.BoundingRect();
		rgn1.Union(rgn2);
		bounds.Resize(3,5);
		test(rgn1.BoundingRect()==bounds);
		rgn1Bounds.Shrink(3,5);
		rgn1Bounds.Resize(8,12);
		test(rgn3.BoundingRect()==rgn1Bounds);
		}
	rgn1.Close();
	rgn2.Close();
	rgn3.Close();
	tmp_rgn.Close();

	RRegionBuf<8> rgnBuf1,rgnBuf2,rgnBuf3,tmp_rgnBuf;

	// Test RRegionBuf (can transform from using a static to using a dynamic buffer).
	rgnBuf1.AddRect(xrect);
	if (!rgnBuf1.IsEmpty())
		{
		for(index=0;index<(sizeof(rect)/sizeof(rect[0]));index++)
			tmp_rgnBuf.AddRect(rect[index]);
		test(tmp_rgnBuf.Count()==4);
		rgnBuf1.Union(tmp_rgnBuf);
		test(rgnBuf1.Count()==7);
		rgn1Bounds = rgnBuf1.BoundingRect();
		rgnBuf2.Copy(rgnBuf1);
		rgnBuf2.Offset(3,5);
		rgnBuf3.Copy(rgnBuf1);
		rgnBuf3.Offset(5,7);
		rgnBuf3.Union(rgnBuf2);
		test(rgnBuf3.Count()==17);
		bounds=rgnBuf1.BoundingRect();
		rgnBuf1.Union(rgnBuf2);
		bounds.Resize(3,5);
		test(rgnBuf1.BoundingRect()==bounds);
		rgn1Bounds.Shrink(3,5);
		rgn1Bounds.Resize(8,12);
		test(rgnBuf3.BoundingRect()==rgn1Bounds);
		}
	rgnBuf1.Close();
	rgnBuf2.Close();
	rgnBuf3.Close();
	tmp_rgnBuf.Close();
	}

void TestRRegion::TestClipRect()
	{
	RRegion rgn1(xrect);
	if (!rgn1.IsEmpty())
		{
		TUint index;
		for(index=0;index<(sizeof(rect)/sizeof(rect[0]));index++)
			rgn1.AddRect(rect[index]);
		TRect clip=rgn1.BoundingRect();
		rgn1.ClipRect(clip);
		test(clip==rgn1.BoundingRect());
		clip.iTl.iX>>=1;
		clip.iTl.iY>>=1;
		clip.iBr.iX>>=1;
		clip.iBr.iY>>=1;
		rgn1.ClipRect(clip);
		test(clip==rgn1.BoundingRect());
		clip.iTl.iX=clip.iBr.iX;
		rgn1.ClipRect(clip);
		test(rgn1.Count()==0);
		}
	rgn1.Close();
	}

void TestRRegion::TestContains()
	{
	RRegion rgn;
	rgn.AddRect(TRect(10,10,20,20));
	rgn.AddRect(TRect(10,20,50,30));
	test(rgn.Contains(TPoint(10,10)));
	test(rgn.Contains(TPoint(49,29)));
	test(rgn.Contains(TPoint(15,15)));
	test(rgn.Contains(TPoint(31,22)));
	test(rgn.Contains(TPoint(50,30))==EFalse);
	test(rgn.Contains(TPoint(-100,-30))==EFalse);
	test(rgn.Contains(TPoint(200,20))==EFalse);
	test(rgn.Contains(TPoint(15,30000))==EFalse);
	rgn.Close();
	TRegionFix<1> rgn2(TRect(20,20,25,25));
	test(rgn2.Contains(TPoint(20,20)));
	test(rgn2.Contains(TPoint(22,23)));
 	test(rgn2.Contains(TPoint(0,0))==EFalse);
	test(rgn2.Contains(TPoint(25,25))==EFalse);
	test(rgn2.Contains(TPoint(30,30))==EFalse);
	}

void TestRRegion::TestIntersects()
	{
	RRegion rgn;
	rgn.AddRect(TRect(10,10,20,20));
	rgn.AddRect(TRect(10,20,50,30));
	test(rgn.Intersects(TRect(10,10,20,20)));
	test(rgn.Intersects(TRect(5,5,15,15)));
	test(rgn.Intersects(TRect(10,20,50,30)));
	test(rgn.Intersects(TRect(10,10,20,20)));
	test(rgn.Intersects(TRect(40,10,60,30)));
	test(rgn.Intersects(TRect(0,0,10,10))==EFalse);
	test(rgn.Intersects(TRect(30,10,40,20))==EFalse);
	rgn.Close();
	TRegionFix<1> rgn2(TRect(20,20,30,30));
	test(rgn2.Intersects(TRect(20,20,30,30)));
	test(rgn2.Intersects(TRect(15,25,25,35)));
	test(rgn2.Intersects(TRect(25,15,35,25)));
	test(rgn2.Intersects(TRect(15,15,25,25)));
	test(rgn2.Intersects(TRect(25,25,35,35)));
	test(rgn2.Intersects(TRect(10,20,20,30))==EFalse);
	test(rgn2.Intersects(TRect(30,20,40,30))==EFalse);
	test(rgn2.Intersects(TRect(20,10,30,20))==EFalse);
	test(rgn2.Intersects(TRect(20,30,30,40))==EFalse);
	// empty rectangles
	test(rgn2.Intersects(TRect(30,30,20,20))==EFalse);
	TRegionFix<1> rgn3(TRect(30,30,20,20));
	test(rgn3.Intersects(TRect(30,30,20,20))==EFalse);
	test(rgn3.Intersects(TRect(20,20,30,30))==EFalse);
	}

void TestRRegion::TestErrors()
	{
	RRegion rgnErr,rgnErr2;
	RRegion rgn;
	TRect rect(1,2,3,4),rect2;
	TPoint pnt;

	rgnErr.ForceError();
	rgn.AddRect(rect);
	rgnErr.Copy(rgn);
	test(rgnErr.CheckError()==EFalse);
	test(rgnErr.Count()==1);

	rgnErr.ForceError();
	test(rgnErr.CheckError()==TRUE);
	rgnErr.AddRect(rect);
	rgnErr.AddRect(TRect(2,3,4,5));
	test(rgnErr.CheckError()==TRUE);
	rgnErr.SubRect(rect);
	test(rgnErr.CheckError()==TRUE);
	rgn.Copy(rgnErr);
	test(rgn.CheckError()==TRUE);
	rgnErr.Offset(1,2);
	rgnErr.Offset(pnt);
	test(rgnErr.CheckError()==TRUE);

	rgn.Union(rgnErr);
	test(rgn.CheckError()==TRUE);
	rgnErr.AddRect(rect);
	test(rgnErr.CheckError()==TRUE);
	rgn.Clear();
	rgn.AddRect(rect);
	rgnErr.Union(rgn);
	test(rgnErr.CheckError()==TRUE);
	rgn.Clear();
	test(rgn.CheckError()==FALSE);

	rgnErr2.Clear();
	rgnErr2.AddRect(rect);
	rgn.Intersection(rgnErr,rgnErr2);
	test(rgn.CheckError()==TRUE);
	rgn.Clear();
	rgn.Intersection(rgnErr2,rgnErr);
	test(rgn.CheckError()==TRUE);
	rgnErr2.ForceError();
	rgn.Clear();
	rgn.Intersection(rgnErr2,rgnErr);
	test(rgn.CheckError()==TRUE);
	rgn.Clear();
	rgn.AddRect(rect);
	rgn.Intersect(rgnErr);
	test(rgn.CheckError()==TRUE);
	rgn.Clear();
	rgn.AddRect(rect);
	rgnErr.Intersect(rgn);
	test(rgnErr.CheckError()==TRUE);
	test(rgn.CheckError()==FALSE);

	test(rgnErr.IsEmpty()==FALSE);

	rgn.Clear();
	rgn.AddRect(rect);
	rgn.SubRegion(rgnErr);
	test(rgn.CheckError()==TRUE);
	test(rgnErr.CheckError()==TRUE);

	rgnErr.ClipRect(rect);
	test(rgnErr.CheckError()==TRUE);
	rgnErr.Tidy();
	test(rgnErr.CheckError()==TRUE);
	rect2=rgnErr.BoundingRect();
	test(rect2.iTl.iX==0 && rect2.iBr.iY==0);

	test(rgnErr.Count()==0);
	rgn.Close();
	rgnErr.Close();
	rgnErr2.Close();
	}

void TestRRegion::doTestSort(RRegion &rgn)
	{
	TInt loop,loop2;
	TRect const rlist[8]={	// 8 Rectangles that form a square with the centre rectangle missing
		TRect(20,30,30,40),
		TRect(20,10,30,20),
		TRect(10,20,20,30),
		TRect(30,20,40,30),
		TRect(10,30,20,40),
		TRect(30,30,40,40),
		TRect(10,10,20,20),
		TRect(30,10,40,20)};
	TRect sorted[8];
	TRect const* plist;
	TRect tmp;

// Work out wot the sorted list should be
	for(loop=0;loop<8;loop++)
		sorted[loop]=rlist[loop];
	for(loop=0;loop<8;loop++)
		{
restart:
		for(loop2=loop+1;loop2<8;loop2++)
			{
			if (sorted[loop2].iTl.iY>sorted[loop].iTl.iY)
				continue;
			if (sorted[loop2].iTl.iY==sorted[loop].iTl.iY && sorted[loop2].iTl.iX>sorted[loop].iTl.iX)
				continue;
			tmp=sorted[loop];
			sorted[loop]=sorted[loop2];
			sorted[loop2]=tmp;
			goto restart;
			}	
		}
	for(loop=0;loop<8;loop++)
		rgn.AddRect(rlist[loop]);
	rgn.Sort();
	plist=rgn.RectangleList();
	for(loop=0;loop<8;loop++)
		test(plist[loop]==sorted[loop]);
	rgn.Close();
	}

void TestRRegion::TestSort()
	{
	RRegion rgn;
	doTestSort(rgn);
	RRegionBuf<1> rgnBuf;
	doTestSort(rgnBuf);
	}

void TestRRegion::doTestRegionBuf(RRegion &aRegion)
	{
	for(TInt index=0;index<8;index++)
		{
		aRegion.AddRect(TRect(index*2,index*2,index*2+2,index*2+2));
		test(aRegion.Count()==(index+1));
		const TRect *pr=aRegion.RectangleList();
		for(TInt index2=0;index2<=index;index2++)
			test(pr[index2]==TRect(index2*2,index2*2,index2*2+2,index2*2+2));
		}
	aRegion.Close();
	}

void TestRRegion::TestRegionBuf()
	{
	RRegionBuf<1> rgn(TRect(1,2,3,4));
	test(rgn[0]==TRect(1,2,3,4));
	RRegionBuf<1> rgn1;
	doTestRegionBuf(rgn1);
	RRegionBuf<2> rgn2;
	doTestRegionBuf(rgn2);
	RRegionBuf<5> rgn5;
	doTestRegionBuf(rgn5);
	RRegionBuf<10> rgn10;
	doTestRegionBuf(rgn10);
	}

void TestRRegion::TestIsContainedBy()
	{
	TRect reference(5,5,90,90); // this is the one to compare against
	TRect outer1(5,5,91,91);   // ref fits inside
	TRect outer2(8,8,91,91); // reference is overlapping
	TRect inner1(6,6,89,89); // this is inside ref

	RRegion rRgn[1];
	TRegion* tRgn[1]={&rRgn[0]};
	
	tRgn[0]->AddRect(reference);

	test(tRgn[0]->IsContainedBy(outer1));
	test(tRgn[0]->IsContainedBy(outer2)==EFalse);
	test(tRgn[0]->IsContainedBy(inner1)==EFalse);
	tRgn[0]->Clear();
	rRgn[0].Close();
	}

void TestRRegion::TestDestroy()
	{
	TInt count=1;
	// allocate from heap
	TRect* pList = new TRect(1,2,3,4);
	RRegion* rgnPtr = new RRegion(count, pList, 5 /*EDefaultGranularity*/ );
	test(rgnPtr->Count()==count);
	
	// destroy the heap based object
	rgnPtr->Destroy();
	}

// Top level test code
LOCAL_C void test_region(TestRRegion t)
	{
	test.Start(_L("Setting values"));
	t.TestSet();
	test.Next(_L("TRegionFix"));
	t.TestRegionFix();
	test.Next(_L("AddRect"));
	t.TestAddRect();
	test.Next(_L("SubRect"));
	t.TestSubRect();
	test.Next(_L("SubRegion"));
	t.TestSubRegion();
	test.Next(_L("Tidy"));
	t.TestTidy();
	test.Next(_L("Spare"));
	t.TestSpare();
	test.Next(_L("Offset"));
	t.TestOffset();
	test.Next(_L("Intersection"));
	t.TestIntersection();
	test.Next(_L("Union"));
	t.TestUnion();
	test.Next(_L("Clip rect"));
	t.TestClipRect();
	test.Next(_L("Contains"));
	t.TestContains();
	test.Next(_L("Intersects"));
	t.TestIntersects();
	test.Next(_L("Errors"));
	t.TestErrors();
	test.Next(_L("Sort"));
	t.TestSort();
	test.Next(_L("RegionBuf"));
	t.TestRegionBuf();
	test.Next(_L("IsContainedBy"));
	t.TestIsContainedBy();
	test.Next(_L("Destroy"));
	t.TestDestroy();
	test.End();
	}

GLDEF_C TInt E32Main()
//
// Main
//
    {

	test.Title();
	__UHEAP_MARK;
	TestRRegion t1(10,20,30,40);
	TestRRegion t2(0,0,1,1);
	TestRRegion t3(1,1,1,1);
	TestRRegion t4(1000000,2000000,3000000,4000000);

	test.Start(_L("class RRegion 1"));
	test_region(t1);
	test.Next(_L("class RRegion 2"));
	test_region(t2);
	test.Next(_L("class RRegion 3"));
	test_region(t3);
	test.Next(_L("class RRegion 4"));
	test_region(t4);
	test.End();
	__UHEAP_MARKEND;
	return(0);
    }


