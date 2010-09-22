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
// e32test\buffer\t_graph.cpp
// Overview:
// Test the TPoint, TSize, TRect class.
// API Information:
// TPoint, TSize, TRect.
// Details:
// - Initialize some two-dimensional points in Cartesian co-ordinates with various 
// positive, negative values. Add and subtract different initialized points and 
// check that results are as expected.
// - Create some two-dimensional sizes, set with various positive, negative values. 
// Add and subtract different initialized sizes and check that results are as expected.
// - Initialize some rectangles in different positions, set various widths, heights and 
// sizes and verify that values are assigned as specified. 
// - Move the rectangle by adding two-dimensional point offset, some x, y offset and check 
// that it is as expected.
// - Resize, shrink, grow, rectangles by adding, subtracting some horizontal, vertical 
// offset and verify that it is as expected.
// - Get the minimal rectangles which bounds one rectangle and the specified rectangles, 
// assign it to the rectangles and check that it is as expected.
// - Assign some rectangles with different positions and test whether the rectangle is 
// empty, overlaps as expected.
// - Get the area of intersection between one rectangle and the specified rectangle, 
// assign it to the rectangle and verify that the assigned rectangle is not empty.
// - Create some rectangles, check that the rectangle's width and height have positive
// values, the rectangle is normalized, whether a point is located within the rectangle 
// and is empty.
// - Create some rectangles, get the points at the centre of the rectangles and check that 
// it is as expected.
// - Create  some three-dimensional points in Cartesian co-ordinates with various 
// positive, negative values. Add and subtract different initialized points and 
// check that results are as expected.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_GRAPH"));

class TestTPoint
	{
public:
	TestTPoint(TInt x1,TInt y1,TInt x2,TInt y2,TInt x3,TInt y3,TInt x4,TInt y4);
	void TestSet();
	void TestAdd();
	void TestSub();
	void TestUnaryMinus();
private:
	TInt iX1;
	TInt iY1;
	TInt iX2;
	TInt iY2;
	TInt iX3;
	TInt iY3;
	TInt iX4;
	TInt iY4;
	};

class TestTSize
	{
public:
	TestTSize(TInt wid1,TInt hgt1,TInt wid2,TInt hgt2,TInt wid3,TInt hgt3,TInt wid4,TInt hgt4);
	void TestSet();
	void TestAdd();
	void TestSub();
	void TestUnaryMinus();
private:
	TInt iWidth1;
	TInt iHeight1;
	TInt iWidth2;
	TInt iHeight2;
	TInt iWidth3;
	TInt iHeight3;
	TInt iWidth4;
	TInt iHeight4;
	};

class TestTRect
	{
public:
	TestTRect(TInt tlx1,TInt tly1,TInt brx1,TInt bry1,
			  TInt tlx2,TInt tly2,TInt brx2,TInt bry2,
			  TInt tlx3,TInt tly3,TInt brx3,TInt bry3);
	void TestSet();
	void TestMove();
	void TestResize();
	void TestShrink();
	void TestGrow();
	void TestBoundingRect();
	void TestIsEmpty();
	void TestIntersects();
	void TestIntersection();
	void TestNormalize();
	void TestContains();
	void TestCenter();
private:
	TInt iTlx1;
	TInt iTly1;
	TInt iBrx1;
	TInt iBry1;
	TInt iTlx2;
	TInt iTly2;
	TInt iBrx2;
	TInt iBry2;
	TInt iTlx3;
	TInt iTly3;
	TInt iBrx3;
	TInt iBry3;
	};

class TestTPoint3D
	{
public:
	TestTPoint3D(TInt x1,TInt y1,TInt z1,TInt x2,TInt y2,TInt z2,TInt x3,TInt y3,TInt z3,TInt x4,TInt y4,TInt z4);
	void TestSet();
	void TestAdd();
	void TestSub();
	void TestUnaryMinus();
private:
	TInt iX1;
	TInt iY1;
	TInt iZ1;
	TInt iX2;
	TInt iY2;
	TInt iZ2;
	TInt iX3;
	TInt iY3;
	TInt iZ3;
	TInt iX4;
	TInt iY4;
	TInt iZ4;

	};

//////////////////////////////////////////////////////////////////////////////
// Point test code															//
//////////////////////////////////////////////////////////////////////////////

TestTPoint::TestTPoint(TInt x1,TInt y1,TInt x2,TInt y2,TInt x3,TInt y3,TInt x4,TInt y4)
	{
	iX1=x1;
	iY1=y1;
	iX2=x2;
	iY2=y2;
	iX3=x3;
	iY3=y3;
	iX4=x4;
	iY4=y4;
	}

void TestTPoint::TestSet()
	{
	TSize s1(iX3,iY3);
	TPoint p1(iX1,iY1);
	TPoint p2(iX2,iY2);
	TPoint p3;
	TPoint p4;

	test(p1.iX==iX1 && p1.iY==iY1);
	test(p2.iX==iX2 && p2.iY==iY2);
	test(p4.iX==0 && p4.iY==0);

	p3.iX=iX3;
	p3.iY=iY3;
	test(p3.iX==iX3 && p3.iY==iY3);
	test(p3.AsSize()==s1);

	p4.SetXY(iX4,iY4);
	test(p4.iX==iX4 && p4.iY==iY4);

	p1=p4;
	test(p1==p4);
	test(p1.iX==p4.iX && p1.iY==p4.iY);

	p3=p2;
	test(p3==p2);
	test(p3.iX==p2.iX && p3.iY==p2.iY);

	p1=p4;
	p1.iX++;
	test(p1!=p4);
	p1=p4;
	p1.iY++;
	test(p1!=p4);
	}

void TestTPoint::TestAdd()
	{
	TSize s1(iX3,iY3);
	TPoint p1(iX1,iY1);
	TPoint p2(iX2,iY2);
	TPoint p3(iX3,iY3);
	TPoint p4(iX4,iY4);
	TPoint px;

	p1+=p2;
	test(p1.iX==(p2.iX+iX1) && p1.iY==(p2.iY+iY1));
	px=p3=p1+p2;
	test(p3.iX==(p1.iX+p2.iX) && p3.iY==(p1.iY+p2.iY));
	test(p3==(p1+p2));
	test(px==p3);
	px=p4+=p1;
	test(p4.iX==(p1.iX+iX4) && p4.iY==(p1.iY+iY4));
	test(px==p4);

	px=p1;
	p1+=s1;
	test(p1.iX==(px.iX+s1.iWidth) && p1.iY==(px.iY+s1.iHeight));
	p2=px+s1;
	test(p2==p1);
	}

void TestTPoint::TestSub()
	{
	TSize s1(iX3,iY3);
	TPoint p1(iX1,iY1);
	TPoint p2(iX2,iY2);
	TPoint p3(iX3,iY3);
	TPoint p4(iX4,iY4);
	TPoint px;

	p1-=p2;
	test(p1.iX==(iX1-p2.iX) && p1.iY==(iY1-p2.iY));
	px=p3=p1-p2;
	test(p3.iX==(p1.iX-p2.iX) && p3.iY==(p1.iY-p2.iY));
	test(p3==(p1-p2));
	test(px==p3);
	px=p4-=p1;
	test(p4.iX==(iX4-p1.iX) && p4.iY==(iY4-p1.iY));
	test(px==p4);

	px=p1;
	p1-=s1;
	test(p1.iX==(px.iX-s1.iWidth) && p1.iY==(px.iY-s1.iHeight));
	p2=px-s1;
	test(p2==p1);
	}

void TestTPoint::TestUnaryMinus()
	{
	TPoint p1(iX1,iY1);
	TPoint p2(iX2,iY2);
	TPoint p3(iX3,iY3);
	TPoint p4(iX4,iY4);

	TPoint p=-p1;
	test (p==TPoint(-iX1,-iY1));
	p=-p2;
	test (p==TPoint(-iX2,-iY2));
	p=-p3;
	test (p==TPoint(-iX3,-iY3));
	p=-p4;
	test (p==TPoint(-iX4,-iY4));
	}

//////////////////////////////////////////////////////////////////////////////
// Size test code															//
//////////////////////////////////////////////////////////////////////////////

TestTSize::TestTSize(TInt x1,TInt y1,TInt x2,TInt y2,TInt x3,TInt y3,TInt x4,TInt y4)
	{
	iWidth1=x1;
	iHeight1=y1;
	iWidth2=x2;
	iHeight2=y2;
	iWidth3=x3;
	iHeight3=y3;
	iWidth4=x4;
	iHeight4=y4;
	}

void TestTSize::TestSet()
	{
	TPoint p1(iWidth3,iHeight3);
	TSize s1(iWidth1,iHeight1);
	TSize s2(iWidth2,iHeight2);
	TSize s3;
	TSize s4;

	test(s1.iWidth==iWidth1 && s1.iHeight==iHeight1);
	test(s2.iWidth==iWidth2 && s2.iHeight==iHeight2);
	test(s4.iWidth==0 && s4.iHeight==0);

	s3.iWidth=iWidth3;
	s3.iHeight=iHeight3;
	test(s3.iWidth==iWidth3 && s3.iHeight==iHeight3);
	test(s3.AsPoint()==p1);

	s4.SetSize(iWidth4,iHeight4);
	test(s4.iWidth==iWidth4 && s4.iHeight==iHeight4);

	s1=s4;
	test(s1==s4);
	test(s1.iWidth==s4.iWidth && s1.iHeight==s4.iHeight);
	s3=s2;
	test(s3==s2);
	test(s3.iWidth==s2.iWidth && s3.iHeight==s2.iHeight);

	s1=s4;
	s1.iWidth++;
	test(s1!=s4);
	s1=s4;
	s1.iHeight++;
	test(s1!=s4);
	}

void TestTSize::TestAdd()
	{
	TPoint p1(iWidth3,iHeight3);
	TSize s1(iWidth1,iHeight1);
	TSize s2(iWidth2,iHeight2);
	TSize s3(iWidth3,iHeight3);
	TSize s4(iWidth4,iHeight4);
	TSize sx;

	s1+=s2;
	test(s1.iWidth==(s2.iWidth+iWidth1) && s1.iHeight==(s2.iHeight+iHeight1));
	sx=s3=s1+s2;
	test(s3.iWidth==(s1.iWidth+s2.iWidth) && s3.iHeight==(s1.iHeight+s2.iHeight));
	test(s3==(s1+s2));
	test(sx==s3);
	sx=s4+=s1;
	test(s4.iWidth==(s1.iWidth+iWidth4) && s4.iHeight==(s1.iHeight+iHeight4));
	test(sx==s4);

	sx=s1;
	s1+=p1;
	test(s1.iWidth==(sx.iWidth+p1.iX) && s1.iHeight==(sx.iHeight+p1.iY));
	s2=sx+p1;
	test(s1==s2);
	}

void TestTSize::TestSub()
	{
	TPoint p1(iWidth3,iHeight3);
	TSize s1(iWidth1,iHeight1);
	TSize s2(iWidth2,iHeight2);
	TSize s3(iWidth3,iHeight3);
	TSize s4(iWidth4,iHeight4);
	TSize sx;

	s1-=s2;
	test(s1.iWidth==(iWidth1-s2.iWidth) && s1.iHeight==(iHeight1-s2.iHeight));
	sx=s3=s1-s2;
	test(s3.iWidth==(s1.iWidth-s2.iWidth) && s3.iHeight==(s1.iHeight-s2.iHeight));
	test(s3==(s1-s2));
	test(sx==s3);
	sx=s4-=s1;
	test(s4.iWidth==(iWidth4-s1.iWidth) && s4.iHeight==(iHeight4-s1.iHeight));
	test(sx==s4);

	sx=s1;
	s1-=p1;
	test(s1.iWidth==(sx.iWidth-p1.iX) && s1.iHeight==(sx.iHeight-p1.iY));
	s2=sx-p1;
	test(s1==s2);
	}

void TestTSize::TestUnaryMinus()
	{
	TSize s1(iWidth1,iHeight1);
	TSize s2(iWidth2,iHeight2);
	TSize s3(iWidth3,iHeight3);
	TSize s4(iWidth4,iHeight4);

	TSize s=-s1;
	test (s==TSize(-iWidth1,-iHeight1));
	s=-s2;
	test (s==TSize(-iWidth2,-iHeight2));
	s=-s3;
	test (s==TSize(-iWidth3,-iHeight3));
	s=-s4;
	test (s==TSize(-iWidth4,-iHeight4));
	}

//////////////////////////////////////////////////////////////////////////////
// Rectangle test code														//
//////////////////////////////////////////////////////////////////////////////

TestTRect::TestTRect(TInt tlx1,TInt tly1,TInt brx1,TInt bry1,
			  				 TInt tlx2,TInt tly2,TInt brx2,TInt bry2,
			  				 TInt tlx3,TInt tly3,TInt brx3,TInt bry3)
	{
	iTlx1=tlx1;
	iTly1=tly1;
	iBrx1=brx1;
	iBry1=bry1;
	iTlx2=tlx2;
	iTly2=tly2;
	iBrx2=brx2;
	iBry2=bry2;
	iTlx3=tlx3;
	iTly3=tly3;
	iBrx3=brx3;
	iBry3=bry3;
	}

void TestTRect::TestSet()
	{
	TSize s1(iTlx3,iTly3);
	TPoint pTl(iTlx2,iTly2);
	TPoint pBr(iBrx2,iBry2);
	TRect r1(iTlx1,iTly1,iBrx1,iBry1);
	TRect r2(pTl,pBr);
	TRect r3;
	TRect r4(pTl,s1);

	test(r1.iTl.iX==iTlx1 && r1.iTl.iY==iTly1 && r1.iBr.iX==iBrx1 && r1.iBr.iY==iBry1);
	test(r2.iTl.iX==iTlx2 && r2.iTl.iY==iTly2 && r2.iBr.iX==iBrx2 && r2.iBr.iY==iBry2);
	test(r2.iTl==pTl && r2.iBr==pBr);
	test(r3.iTl.iX==0 && r3.iTl.iY==0 && r3.iBr.iX==0 && r3.iBr.iY==0);
	test(r4.iTl==pTl && r4.iBr.iX==(pTl.iX+s1.iWidth) && r4.iBr.iY==(pTl.iY+s1.iHeight));

	r3.iTl.iX=iTlx3;
	r3.iTl.iY=iTly3;
	r3.iBr.iX=iBrx3;
	r3.iBr.iY=iBry3;
	test(r3.iTl.iX==iTlx3 && r3.iTl.iY==iTly3 && r3.iBr.iX==iBrx3 && r3.iBr.iY==iBry3);

	r1.SetRect(iTlx2,iTly2,iBrx2,iBry2);
	test(r1==r2);

	r2=r3;
	test(r2==r3);

	pTl.SetXY(iTlx3,iTly3);
	pBr.SetXY(iBrx3,iBry3);
	r1.SetRect(pTl,pBr);
	test(r1.iTl.iX==iTlx3 && r1.iTl.iY==iTly3 && r1.iBr.iX==iBrx3 && r1.iBr.iY==iBry3);
	pTl.SetXY(iTlx2,iTly2);
	r1.SetRect(pTl,s1);
	test(r1==r4);
	r4.iTl.iX++;
	test(r1!=r4);
	r4.iTl.iX--;
	test(r1==r4);
	r4.iTl.iY++;
	test(r1!=r4);
	r4.iTl.iY--;
	test(r1==r4);
	r4.iBr.iX++;
	test(r1!=r4);
	r4.iBr.iX--;
	test(r1==r4);
	r4.iBr.iY++;
	test(r1!=r4);

	r4=TRect(0,0,0,0);
	r4.SetWidth(r1.Size().iWidth);
	test(r1.Size().iWidth==r4.Size().iWidth);
	test(r4.Size().iHeight==0);

	r4.SetHeight(r1.Size().iHeight);
	test(r1.Size().iHeight==r4.Size().iHeight);
	test(r1.Size().iWidth==r4.Size().iWidth);

	r4=TRect(0,0,0,0);
	r4.SetSize(r1.Size());
	test(r1.Size()==r4.Size());
	}

void TestTRect::TestMove()
	{
	TPoint offs(iTlx2,iTly2);
	TRect r1(iTlx1,iTly1,iBrx1,iBry1);
	TRect rCheck,rTest;

	rCheck=r1;
	rCheck.iTl+=offs;
	rCheck.iBr+=offs;

	rTest=r1;
	rTest.Move(offs);
	test(rTest==rCheck);

	rTest=r1;
	rTest.Move(offs.iX,offs.iY);
	test(rTest==rCheck);
	}

void TestTRect::TestResize()
	{
	TSize resize(iTlx1,iTly1);
	TRect rTest(iTlx2,iTly2,iBrx2,iBry2);
	TRect rCheck;

	rCheck=rTest;
	rTest.Resize(resize);
	rCheck.iBr+=resize;
	test(rTest==rCheck);
	}

void TestTRect::TestShrink()
	{
	TSize shrink(iTlx3,iTly3);
	TRect r2(iTlx2,iTly2,iBrx2,iBry2);
	TRect rCheck,rTest;

	rCheck=r2;
	rCheck.iTl+=shrink;
	rCheck.iBr-=shrink;

	rTest=r2;
	rTest.Shrink(shrink);
	test(rTest==rCheck);

	rTest=r2;
	rTest.Shrink(shrink.iWidth,shrink.iHeight);
	test(rTest==rCheck);
	}

void TestTRect::TestGrow()
	{
	TSize grow(iTlx3,iTly3);
	TRect r2(iTlx2,iTly2,iBrx2,iBry2);
	TRect rCheck,rTest;

	rCheck=r2;
	rCheck.iTl-=grow;
	rCheck.iBr+=grow;

	rTest=r2;
	rTest.Grow(grow.iWidth,grow.iHeight);
	test(rTest==rCheck);

	rTest=r2;
	rTest.Grow(grow);
	test(rTest==rCheck);
	}

void TestTRect::TestBoundingRect()
	{
	TRect r1(iTlx1,iTly1,iBrx1,iBry1);
	TRect r2(iTlx2,iTly2,iBrx2,iBry2);
	TRect r3(iTlx3,iTly3,iBrx3,iBry3);
	TRect rCheck,rTest;

	rCheck=r1;
	rCheck.iTl.iX=Min(r1.iTl.iX,r2.iTl.iX);
	rCheck.iTl.iY=Min(r1.iTl.iY,r2.iTl.iY);
	rCheck.iBr.iX=Max(r1.iBr.iX,r2.iBr.iX);
	rCheck.iBr.iY=Max(r1.iBr.iY,r2.iBr.iY);
	rTest=r1;
	rTest.BoundingRect(r2);
	test(rTest==rCheck);

	rCheck=r1;
	rCheck.iTl.iX=Min(r1.iTl.iX,r3.iTl.iX);
	rCheck.iTl.iY=Min(r1.iTl.iY,r3.iTl.iY);
	rCheck.iBr.iX=Max(r1.iBr.iX,r3.iBr.iX);
	rCheck.iBr.iY=Max(r1.iBr.iY,r3.iBr.iY);
	rTest=r1;
	rTest.BoundingRect(r3);
	test(rTest==rCheck);
	}

void TestTRect::TestIsEmpty()
	{
	TRect f1(0,0,1,1);			// f? should return false
	TRect f2(-1,-1,0,0);
	TRect f3(-2,-2,-1,-1);
	TRect t1(0,0,1,0);			// t? should return true
	TRect t2(0,0,0,1);
	TRect t3(0,0,-1,1);
	TRect t4(0,0,1,-1);
	TRect t5(-2,-2,-1,-3);
	TRect t6(-2,-2,-3,-1);

	test(f1.IsEmpty()==FALSE);
	test(f2.IsEmpty()==FALSE);
	test(f3.IsEmpty()==FALSE);
	test(t1.IsEmpty()==TRUE);
	test(t2.IsEmpty()==TRUE);
	test(t3.IsEmpty()==TRUE);
	test(t4.IsEmpty()==TRUE);
	test(t5.IsEmpty()==TRUE);
	test(t6.IsEmpty()==TRUE);
	}

void TestTRect::TestIntersects()
	{
	test(TRect(-5,-10,5,10).Intersects(TRect(4,-10,10,10))==TRUE);
	test(TRect(-5,-10,5,10).Intersects(TRect(5,-10,10,10))==FALSE);
	test(TRect(-5,-10,5,10).Intersects(TRect(6,-10,10,10))==FALSE);
	test(TRect(-5,-10,5,10).Intersects(TRect(-5, 9, 5,20))==TRUE);
	test(TRect(-5,-10,5,10).Intersects(TRect(-5,10, 5,20))==FALSE);
	test(TRect(-5,-10,5,10).Intersects(TRect(-5,11, 5,20))==FALSE);
//
	test(TRect(4,-10,10,10).Intersects(TRect(-5,-10,5,10))==TRUE);
	test(TRect(5,-10,10,10).Intersects(TRect(-5,-10,5,10))==FALSE);
	test(TRect(6,-10,10,10).Intersects(TRect(-5,-10,5,10))==FALSE);
	test(TRect(-5, 9, 5,20).Intersects(TRect(-5,-10,5,10))==TRUE);
	test(TRect(-5,10, 5,20).Intersects(TRect(-5,-10,5,10))==FALSE);
	test(TRect(-5,11, 5,20).Intersects(TRect(-5,-10,5,10))==FALSE);
// Empty rects, should all return FALSE
	test(TRect(0,11, 5,11).Intersects(TRect(0,0,5,20))==FALSE);
	test(TRect(1,10, 1,11).Intersects(TRect(0,0,5,20))==FALSE);
	test(TRect(5,11, 5,11).Intersects(TRect(0,0,5,20))==FALSE);
	}

void TestTRect::TestIntersection()
	{
	TRect r1(iTlx1,iTly1,iBrx1,iBry1);
	TRect r2(iTlx2,iTly2,iBrx2,iBry2);
	TRect r3(iTlx3,iTly3,iBrx3,iBry3);
	TRect rCheck,rTest;

	rCheck=r1;
	rCheck.iTl.iX=Max(r1.iTl.iX,r2.iTl.iX);
	rCheck.iTl.iY=Max(r1.iTl.iY,r2.iTl.iY);
	rCheck.iBr.iX=Min(r1.iBr.iX,r2.iBr.iX);
	rCheck.iBr.iY=Min(r1.iBr.iY,r2.iBr.iY);
	rTest=r1;
	rTest.Intersection(r2);
	if (!rCheck.IsEmpty())
		test(rTest==rCheck);

	rCheck=r1;
	rCheck.iTl.iX=Max(r1.iTl.iX,r3.iTl.iX);
	rCheck.iTl.iY=Max(r1.iTl.iY,r3.iTl.iY);
	rCheck.iBr.iX=Min(r1.iBr.iX,r3.iBr.iX);
	rCheck.iBr.iY=Min(r1.iBr.iY,r3.iBr.iY);
	rTest=r1;
	rTest.Intersection(r3);
	if (!rCheck.IsEmpty())
		test(rTest==rCheck);

	rCheck=r3;
	rCheck.iTl.iX=Max(r3.iTl.iX,r2.iTl.iX);
	rCheck.iTl.iY=Max(r3.iTl.iY,r2.iTl.iY);
	rCheck.iBr.iX=Min(r3.iBr.iX,r2.iBr.iX);
	rCheck.iBr.iY=Min(r3.iBr.iY,r2.iBr.iY);
	rTest=r3;
	rTest.Intersection(r2);
	if (!rCheck.IsEmpty())
		test(rTest==rCheck);
	}

void TestTRect::TestNormalize()
	{
	TUint i;
    TInt tmp;
	TRect a[]={TRect(0,0,1,1),TRect(-1,0,0,-1),TRect(-2,2,-1,1),TRect(10,2,1,0),
			   TRect(-10,-20,-2,-1),TRect(0,0,10,0),TRect(20,1,20,2),TRect(10,10,10,10)};
	TBool aIsNormalized[]={TRUE,FALSE,FALSE,FALSE,TRUE,TRUE,TRUE,TRUE};
	TRect tst,check;

	for(i=0;i<(sizeof(a)/sizeof(a[0]));i++)
		{
		check=tst=a[i];
		if (check.iTl.iX>check.iBr.iX)
			{
			tmp=check.iBr.iX;
			check.iBr.iX=check.iTl.iX;
			check.iTl.iX=tmp;
			}
		if (check.iTl.iY>check.iBr.iY)
			{
			tmp=check.iBr.iY;
			check.iBr.iY=check.iTl.iY;
			check.iTl.iY=tmp;
			}
		test(tst.IsNormalized()==aIsNormalized[i]);
		tst.Normalize();
		test(tst.IsNormalized()==TRUE);
		test(tst==check);
		}
	}

void TestTRect::TestContains()
	{
	TPoint tl1(iTlx1,iTly1);
	TPoint br1(iBrx1,iBry1);
	TPoint tl2(iTlx2,iTly2);
	TPoint br2(iBrx2,iBry2);
	TPoint tl3(iTlx3,iTly3);
	TPoint br3(iBrx3,iBry3);
	TRect rect1(tl1,br1);
	TRect rect2(tl2,br2);
	TRect rect3(tl3,br3);

	rect1.Normalize();
	rect2.Normalize();
	rect3.Normalize();
	test(rect1.IsEmpty() || rect1.Contains(rect1.iTl));
	test(rect2.IsEmpty() || rect2.Contains(rect2.iTl));
	test(rect3.IsEmpty() || rect3.Contains(rect3.iTl));
	test(!rect1.Contains(rect1.iBr));
	test(!rect2.Contains(rect2.iBr));
	test(!rect3.Contains(rect3.iBr));
	TPoint pnt;
	pnt.SetXY(rect1.iTl.iX,rect1.iBr.iY);
	test(!rect1.Contains(pnt));
	pnt.SetXY(rect1.iBr.iX,rect1.iTl.iY);
	test(!rect1.Contains(pnt));
	pnt=rect2.iTl;
	if (rect2.iBr.iX>(rect2.iTl.iX+1) && rect2.iBr.iY>(rect2.iTl.iY+1))
		pnt+=TPoint(1,1);
	test(rect2.Contains(pnt));
	}

void TestTRect::TestCenter()
	{
	TRect rect[]={TRect(iTlx1,iTly1,iBrx1,iBry1),TRect(iTlx2,iTly2,iBrx2,iBry2),TRect(iTlx3,iTly3,iBrx3,iBry3)};

	for(TUint i=0;i<(sizeof(rect)/sizeof(rect[0]));i++)
		{
		TRect& r=rect[i];
		TPoint center((r.iBr.iX+r.iTl.iX)/2,(r.iBr.iY+r.iTl.iY)/2);
		test(center==r.Center());
		}
	}

//////////////////////////////////////////////////////////////////////////////
// 3DPoint test code														//
//////////////////////////////////////////////////////////////////////////////
TestTPoint3D::TestTPoint3D(TInt x1,TInt y1,TInt z1,TInt x2,TInt y2,TInt z2,TInt x3,TInt y3,TInt z3,TInt x4,TInt y4,TInt z4)
	{
	iX1=x1;
	iY1=y1;
	iZ1=z1;
	iX2=x2;
	iY2=y2;
	iZ2=z2;
	iX3=x3;
	iY3=y3;
	iZ3=z3;
	iX4=x4;
	iY4=y4;
	iZ4=z4;
	}


void TestTPoint3D::TestSet()
	{

	TPoint   aPoint(iX3,iY3);
	TPoint3D p3D1(iX1,iY1,iZ1);
	TPoint3D p3D2(iX2,iY2,iZ2);
	TPoint3D p3D3;
	TPoint3D p3D4;
	TPoint3D p3D5(aPoint);

	test(p3D1.iX==iX1 && p3D1.iY==iY1 && p3D1.iZ==iZ1);
	test(p3D2.iX==iX2 && p3D2.iY==iY2 && p3D2.iZ==iZ2);
	test(p3D4.iX==0 && p3D4.iY==0 && p3D4.iZ==0);

	p3D3.iX=iX3;
	p3D3.iY=iY3;
	p3D3.iZ=iZ3;
	test(p3D3.iX==iX3 && p3D3.iY==iY3&& p3D3.iZ==iZ3);
	test(p3D3.AsPoint()==aPoint);
	test(p3D5.AsPoint()==p3D3.AsPoint());
	
	aPoint.iX=iX2;
	aPoint.iY=iY2;
	TInt z = p3D5.iZ;
	p3D5.SetPoint(aPoint);
	test(p3D5.iX==iX2);
	test(p3D5.iY==iY2);
	test(p3D5.iZ==z);

	p3D4.SetXYZ(iX4,iY4,iZ4);
	test(p3D4.iX==iX4 && p3D4.iY==iY4);

	p3D1=p3D4;
	test(p3D1==p3D4);
	test(p3D1.iX==p3D4.iX && p3D1.iY==p3D4.iY && p3D1.iZ==p3D4.iZ);

	p3D3=p3D2;
	test(p3D3==p3D2);
	test(p3D3.iX==p3D2.iX && p3D3.iY==p3D2.iY && p3D3.iZ==p3D2.iZ);

	p3D1=p3D4;
	p3D1.iX++;
	test(p3D1!=p3D4);
	p3D1=p3D4;
	p3D1.iY++;
	test(p3D1!=p3D4);
	}


void TestTPoint3D::TestAdd()
	{
	TPoint apoint(iX3,iY3);
	TPoint3D p1(iX1,iY1,iZ1);
	TPoint3D p2(iX2,iY2,iZ2);
	TPoint3D p3(iX3,iY3,iZ3);
	TPoint3D p4(iX4,iY4,iZ4);
	TPoint3D px;

	p1+=p2;
	test(p1.iX==(p2.iX+iX1) && p1.iY==(p2.iY+iY1)&& p1.iZ==(p2.iZ+iZ1));
	px=p3=p1+p2;
	test(p3.iX==(p1.iX+p2.iX) && p3.iY==(p1.iY+p2.iY)&& p3.iZ==(p1.iZ+p2.iZ));
	test(p3==(p1+p2));
	test(px==p3);
	px=p4+=p1;
	test(p4.iX==(p1.iX+iX4) && p4.iY==(p1.iY+iY4)&& p4.iZ==(p1.iZ+iZ4));
	test(px==p4);
	px=p1;
	p1+=apoint;
	//TPoint has no Z co-ordinate
	test(p1.iX==(px.iX+apoint.iX) && p1.iY==(px.iY+apoint.iY) &&p1.iZ==px.iZ);
	p2=px+apoint;
	test(p2==p1);
	}

void TestTPoint3D::TestSub()
	{
	TPoint apoint(iX3,iY3);
	TPoint3D p1(iX1,iY1,iZ1);
	TPoint3D p2(iX2,iY2,iZ2);
	TPoint3D p3(iX3,iY3,iZ3);
	TPoint3D p4(iX4,iY4,iZ4);
	TPoint3D px;

	p1-=p2;
	test(p1.iX==(iX1-p2.iX) && p1.iY==(iY1-p2.iY)&& p1.iZ==(iZ1-p2.iZ));
	px=p3=p1-p2;
	test(p3.iX==(p1.iX-p2.iX) && p3.iY==(p1.iY-p2.iY)&& p3.iZ==(p1.iZ-p2.iZ));
	test(p3==(p1-p2));
	test(px==p3);
	px=p4-=p1;
	test(p4.iX==(iX4-p1.iX) && p4.iY==(iY4-p1.iY)&& p4.iZ==(iZ4-p1.iZ));
	test(px==p4);

	px=p1;
	p1-=apoint;
	//TPoint has no Z co-ordinate
	test(p1.iX==(px.iX-apoint.iX) && p1.iY==(px.iY-apoint.iY)&&p1.iZ==px.iZ);
	p2=px-apoint;
	test(p2==p1);
	}

void TestTPoint3D::TestUnaryMinus()
	{
	TPoint3D p1(iX1,iY1,iZ1);
	TPoint3D p2(iX2,iY2,iZ2);
	TPoint3D p3(iX3,iY3,iZ3);
	TPoint3D p4(iX4,iY4,iZ4);
	TPoint3D p=-p1;
	test (p==TPoint3D(-iX1,-iY1,-iZ1));
	p=-p2;
	test (p==TPoint3D(-iX2,-iY2,-iZ2));
	p=-p3;
	test (p==TPoint3D(-iX3,-iY3,-iZ3));
	p=-p4;
	test (p==TPoint3D(-iX4,-iY4,-iZ4));
	}

//////////////////////////////////////////////////////////////////////////////
// Top level test code														//
//////////////////////////////////////////////////////////////////////////////

LOCAL_C void test_point(TestTPoint t)
	{
	test.Start(_L("Setting values"));
	t.TestSet();
	test.Next(_L("Addition"));
	t.TestAdd();
	test.Next(_L("Subtraction"));
	t.TestSub();
	test.Next(_L("Unary minus"));
	t.TestUnaryMinus();
	test.End();
	}

LOCAL_C void test_size(TestTSize t)
	{
	test.Start(_L("Setting values"));
	t.TestSet();
	test.Next(_L("Addition"));
	t.TestAdd();
	test.Next(_L("Subtraction"));
	t.TestSub();
	test.Next(_L("Unary minus"));
	t.TestUnaryMinus();
	test.End();
	}

LOCAL_C void test_rect(TestTRect t)
	{
	test.Start(_L("Setting values"));
	t.TestSet();
	test.Next(_L("Offset"));
	t.TestMove();
	test.Next(_L("Resize"));
	t.TestResize();
	test.Next(_L("Grow"));
	t.TestGrow();
	test.Next(_L("Shrink"));
	t.TestShrink();
	test.Next(_L("BoundingRect"));
	t.TestBoundingRect();
	test.Next(_L("IsEmpty"));
	t.TestIsEmpty();
	test.Next(_L("Intersects"));
	t.TestIntersects();
	test.Next(_L("Intersection"));
	t.TestIntersection();
	test.Next(_L("Normalize"));
	t.TestNormalize();
	test.Next(_L("Contains"));
	t.TestContains();
	test.Next(_L("Center"));
	t.TestCenter();
	test.End();
	}


LOCAL_C void test_3Dpoint(TestTPoint3D t)
	{
	test.Start(_L("Setting values"));
	t.TestSet();
	test.Next(_L("Addition"));
	t.TestAdd();
	test.Next(_L("Subtraction"));
	t.TestSub();
	test.Next(_L("Unary minus"));
	t.TestUnaryMinus();
	test.End();
	}

GLDEF_C TInt E32Main()
    {
//


	test.Title();
//
// Test the TPoint type.
//
	TestTPoint	tp1(1,2, 3,5, 8,13, 20,33);
	TestTPoint	tp2(10,-12, -55,29, -222,-666, 0,0);
	TestTPoint	tp3(345678,10, -9,987654, -1234567,-9876543, 222222,33333);
	TestTPoint	tp4(-1,-2,-3,-4,-5,-6,-7,-8);
	TestTPoint	tp5(0,0, 1000000,0, 0,2000000, 3000000,4000000);
//
	test.Start(_L("class TPoint 1"));
	test_point(tp1);
	test.Next(_L("class TPoint 2"));
	test_point(tp2);
	test.Next(_L("class TPoint 3"));
	test_point(tp3);
	test.Next(_L("class TPoint 4"));
	test_point(tp4);
	test.Next(_L("class TPoint 5"));
	test_point(tp5);
//
// Test the TSize type.
//
	TestTSize	sp1(5,4, 9,5, 17,33, 44,75);
	TestTSize	sp2(12,-10, -29,55, -666,-222, 0,0);
	TestTSize	sp3(456789,20, -3,876543, -2345678,-8765432, 33333,11111);
	TestTSize	sp4(-1,-2,-3,-4,-5,-6,-7,-8);
	TestTSize	sp5(0,0, 2000000,0, 0,3000000, 1000000,6000000);
//
	test.Next(_L("class TSize 1"));
	test_size(sp1);
	test.Next(_L("class TSize 2"));
	test_size(sp2);
	test.Next(_L("class TSize 3"));
	test_size(sp3);
	test.Next(_L("class TSize 4"));
	test_size(sp4);
	test.Next(_L("class TSize 5"));
	test_size(sp5);
//
// TRect tests
//
	TestTRect	tr1(0,0,1,2, 10,20,14,19, 33,11,44,55);
	TestTRect	tr2(-10,-20,11,22, -100,-200,-90,-199, 3000,1100,-4400,-5500);
	TestTRect	tr3(0,0,0,0, 1,1,2,2, -1,-1,0,0);
//
	test.Next(_L("class TRect 1"));
	test_rect(tr1);
	test.Next(_L("class TRect 2"));
	test_rect(tr2);
	test.Next(_L("class TRect 3"));
	test_rect(tr3);
//
//
// Test the TPoint3D type.
//
	TestTPoint3D	t3dp1(1,2,3, 4,5,6, 7 ,8,9, 10,11,12);
	TestTPoint3D	t3dp2(28,-12,75, -55,-98, -222, -788,-666,-155, 0,0,0);
	TestTPoint3D	t3dp3(89823,-45,12121,-78454,345678,10, -9,987654, -1234567,-9876543, 222222,33333);
	TestTPoint3D	t3dp4(-12,-32,-53,-84,-95,-456,-467,-4658,-45,-908,-65,-908);
	TestTPoint3D	t3dp5(0,0,0, 23,1000000,0,10677,-3,2000000, 3000000,4000000,501);

	//
	test.Start(_L("class TPoint3D 1"));
	test_3Dpoint(t3dp1);
	test.Next(_L("class TPoint3D 2"));
	test_3Dpoint(t3dp2);
	test.Next(_L("class TPoint3D 3"));
	test_3Dpoint(t3dp3);
	test.Next(_L("class TPoint3D 4"));
	test_3Dpoint(t3dp4);
	test.Next(_L("class TPoint3D 5"));
	test_3Dpoint(t3dp5);
    //
	test.Printf(_L("T_GRAPH: TEST Successfully Completed \n"));
	test.End();
	test.Close();
	return(0);
    }

