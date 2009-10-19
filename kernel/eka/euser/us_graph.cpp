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
// e32\euser\us_graph.cpp
// 
//

#include "us_std.h"




EXPORT_C TBool TPoint::operator==(const TPoint& aPoint) const
/**
Compares two points for equality.

For two points to be equal, both their x and y co-ordinate values must be 
equal.

@param aPoint The point to be compared with this point.

@return True, if the two points are equal; false, otherwise.
*/
	{

	return(iX==aPoint.iX && iY==aPoint.iY);
	}




EXPORT_C TBool TPoint::operator!=(const TPoint& aPoint) const
/**
Compares two points for inequality.

For two points to be unequal, either their x or their y co-ordinate values 
must be different.

@param aPoint The point to be compared with this point.

@return True, if the two points are unequal; false, otherwise.
*/
	{

	return(iX!=aPoint.iX || iY!=aPoint.iY);
	}




EXPORT_C TPoint& TPoint::operator-=(const TPoint& aPoint)
/**
TPoint subtraction assignment operator.

The operator subtracts the specified point from this point, and assigns the 
result back to this point.

@param aPoint The point to be subtracted.

@return A reference to this point object.
*/
	{

	iX-=aPoint.iX;
	iY-=aPoint.iY;
	return(*this);
	}




EXPORT_C TPoint& TPoint::operator-=(const TSize& aSize)
/**
TSize subtraction assignment operator.

The operator subtracts the specified TSize from this point, and assigns the 
result back to this point.

The operation proceeds by:

1. subtracting the width value of the TSize from the x co-ordinate value

2. subtracting the height value of the TSize from the y co-ordinate value

@param aSize The TSize to be subtracted.

@return A reference to this point object.
*/
	{

	iX-=aSize.iWidth;
	iY-=aSize.iHeight;
	return(*this);
	}




EXPORT_C TPoint& TPoint::operator+=(const TPoint& aPoint)
/**
TPoint addition assignment operator. 

The operator adds the specified point to this point, and assigns the result 
back to this point.

@param aPoint The point to be added.

@return A reference to this point object.
*/
	{

	iX+=aPoint.iX;
	iY+=aPoint.iY;
	return(*this);
	}




EXPORT_C TPoint& TPoint::operator+=(const TSize& aSize)
/**
TSize addition assignment operator. 

The operator adds the specified TSize to this point, and assigns the result 
back to this point.

The operation proceeds by:

1. adding the width value of the TSize to the x co-ordinate value

2. adding the height value of the TSize to the y co-ordinate value

@param aSize The TSize to be added to this point. 

@return A reference to this point object.
*/
	{

	iX+=aSize.iWidth;
	iY+=aSize.iHeight;
	return(*this);
	}




EXPORT_C TPoint TPoint::operator-(const TPoint& aPoint) const
/**
TPoint subtraction operator.

The operator subtracts the specified point from this point, and returns the 
resulting value.

@param aPoint The point to be subtracted from this point. 

@return The result of the operation.
*/
	{

	TPoint ret=* this;
	ret-= aPoint;
	return(ret);
	}




EXPORT_C TPoint TPoint::operator-(const TSize& aSize) const
/**
TSize subtraction operator. 

The operator subtracts the specified TSize from this point, and returns the 
resulting value.

The operation proceeds by:

1. subtracting the width value of the TSize from the x co-ordinate value

2. subtracting the height value of the TSize from the y co-ordinate value.

@param aSize The TSize to be subtracted.

@return The result of the operation.
*/
	{

	TPoint ret=* this;
	ret-= aSize;
	return(ret);
	}




EXPORT_C TPoint TPoint::operator+(const TPoint& aPoint) const
/**
The operator adds the specified point to this point, and returns the resulting 
value.

@param aPoint The point to be added to this point.

@return The result of the operation.
*/
	{

	TPoint ret=* this;
	ret+= aPoint;
	return(ret);
	}




EXPORT_C TPoint TPoint::operator+(const TSize& aSize) const
/**
TSize addition operator. 

The operator adds the specified TSize to this point, and returns the resulting 
value.

The operation proceeds by:

1. adding the width value of the TSize to the x co-ordinate value

2. adding the height value of the TSize to the y co-ordinate value.

@param aSize The TSize to be added to this TPoint. 

@return The result of the operation.
*/
	{

	TPoint ret=* this;
	ret+= aSize;
	return(ret);
	}




EXPORT_C TPoint TPoint::operator-() const
/**
Unary minus operator.

The operator returns the negation of this point.

@return The result of the operation.
*/
	{

	return TPoint(-iX,-iY);
	}




EXPORT_C void TPoint::SetXY(TInt aX,TInt aY)
/**
Sets the x and y co-ordinates for this point.

@param aX The value to assign to the x co-ordinate. 
@param aY The value to assign to the y co-ordinate.
*/
	{

	iX=aX;
	iY=aY;
	}




EXPORT_C TSize TPoint::AsSize() const
/**
Gets the size of the rectangle whose top left hand corner is the origin of 
the screen co-ordinates and whose bottom right hand corner is this point.

@return The co-ordinates of this point converted to a size.
*/
	{
	return(TSize(iX,iY));
	}





EXPORT_C TBool TPoint3D::operator==(const TPoint3D& aPoint3D) const
/**
Compares two 3D points(TPoint3D) for equality.

For two TPoint3D to be equal, their x , y and zco-ordinate values must be 
equal.

@param aPoint3D The point to be compared with this point.

@return True, if the two points are equal; false, otherwise.
*/
	{
	return(iX==aPoint3D.iX && iY==aPoint3D.iY && iZ==aPoint3D.iZ);
	}




EXPORT_C TBool TPoint3D::operator!=(const TPoint3D& aPoint3D) const
/**
Compares two 3D points for inequality.

For two points to be unequal, their x  or y or z co-ordinate values 
must be different.

@param aPoint3D The point to be compared with this point.

@return True, if the two points are unequal; false, otherwise.
*/
	{
	return(iX!=aPoint3D.iX || iY!=aPoint3D.iY || iZ!=aPoint3D.iZ);
	}






EXPORT_C TPoint3D& TPoint3D::operator-=(const TPoint3D& aPoint3D)
/**
TPoint3D subtraction assignment operator.

The operator subtracts the specified point from this point, and assigns the 
result back to this point.

@param aPoint The point to be subtracted.

@return A reference to this point object.
*/
	{
	iX-=aPoint3D.iX;
	iY-=aPoint3D.iY;
	iZ-=aPoint3D.iZ;
	return(*this);
	}



EXPORT_C TPoint3D& TPoint3D::operator-=(const TPoint& aPoint)

/**
TPoint subtraction assignment operator.

The operator subtracts the specified TPoint from this point(TPoint3D), and assigns the 
result back to this point.

The operation proceeds by
subtracting x and y cordinates of the TPoin to this point and no changes  to the Z-coordinatete value

@param aPoint The aPoint to be subtracted.

@return A reference to this point object.
*/
	{
	iX-=aPoint.iX;
	iY-=aPoint.iY;
	//No Changes to the z co-ordinate
	return(*this);
	}



EXPORT_C TPoint3D& TPoint3D::operator+=(const TPoint3D& aPoint3D)
/**
TPoint3D addition assignment operator. 

The operator adds the specified point to this point, and assigns the result 
back to this point.

@param aPoint3D The point to be added.

@return A reference to this point object.
*/
	{
	iX+=aPoint3D.iX;
	iY+=aPoint3D.iY;
	iZ+=aPoint3D.iZ;
	return(*this);
	}

EXPORT_C TPoint3D& TPoint3D::operator+=(const TPoint& aPoint)
/**
TPoint addition assignment operator. 

The operator adds the specified TPoint to this point, and assigns the result 
back to this point.

The operation proceeds by:
adding  x and y cordinates of the TPoin to this point and no changes  to the Z-coordinatete value

@param aPoint The TPoint to be added to this point. 

@return A reference to this point object.
*/
	{

	iX+=aPoint.iX;
	iY+=aPoint.iY;
	//No Changes to the z co-ordinate
	return(*this);
	}
EXPORT_C TPoint3D TPoint3D::operator-(const TPoint3D& aPoint3D) const
/**
TPoint3D subtraction operator.

The operator subtracts the specified point from this point, and returns the 
resulting value.

@param aPoint3D The point to be subtracted from this point. 

@return the point(TPoint3D) which is the  result of the operation.
*/
	{

	TPoint3D ret=* this;
	ret-= aPoint3D;
	return(ret);
	}



EXPORT_C TPoint3D TPoint3D::operator-(const TPoint& aPoint) const
/**
TPoint subtraction operator. 

The operator subtracts the specified TPoint from this point, and returns the 
resulting value.

@param aPoint The TPoint to be subtracted.

@return the point(TPoint3D) which is the  result of the operation.
*/
	{

	TPoint3D ret=* this;
	ret-= aPoint;
	return(ret);
	}


EXPORT_C TPoint3D TPoint3D::operator+(const TPoint3D& aPoint3D) const
/**
The operator adds the specified point to this point, and returns the resulting 
value.

@param aPoint3D The point to be added to this point.

@return the point(TPoint3D) which is the  result of the operation.
*/
	{

	TPoint3D ret=* this;
	ret+= aPoint3D;
	return(ret);
	}

EXPORT_C TPoint3D TPoint3D::operator+(const TPoint& aPoint) const
/**
TPoint addition operator. 

The operator adds the specified TPoint to this point, and returns the resulting 
value.

@param aSize The TSize to be added to this TPoint. 

@return the point(TPoint3D) which is the  result of the operation.
*/
	{

	TPoint3D ret=* this;
	ret+= aPoint;
	return(ret);
	}


EXPORT_C TPoint3D TPoint3D::operator-() const
/**
Unary minus operator.

The operator returns the negation of this point.

@return the point(TPoint3D) which is the  result of Unary minus operation.
*/
	{
	return TPoint3D(-iX,-iY,-iZ);
	}



EXPORT_C void TPoint3D::SetXYZ(TInt aX,TInt aY,TInt aZ)
/**
Sets the x , y and z co-ordinates for this point.

@param aX The value to assign to the x co-ordinate. 
@param aY The value to assign to the y co-ordinate.
@param aZ The value to assign to the z co-ordinate.
*/
	{
	iX=aX;
	iY=aY;
	iZ=aZ;
	}

EXPORT_C void TPoint3D::SetPoint(const TPoint& aPoint)
/*
TPoint3D from TPoint, sets the Z co-ordinate to  Zero
@param aPoint The TPoint to add to this point 
*/
	{
	iX=aPoint.iX;
	iY=aPoint.iY;
	iZ=0;
	}
	

EXPORT_C TPoint TPoint3D::AsPoint() const
/**
Gets Tpoint from Tpoint3D
@return TPoint from X and Y cordinates of Tpoint3D
*/
	{
	return(TPoint(iX,iY));
	}






EXPORT_C TBool TSize::operator==(const TSize& aSize) const
/**
Compares this TSize with the specified TSize for equality. 

For two TSizes to be equal, both their width and height values must be equal.

@param aSize The TSize to be compared with this TSize.

@return True, if the two TSize are equal; false, otherwise.
*/
	{
	return(iWidth==aSize.iWidth && iHeight==aSize.iHeight);
	}




EXPORT_C TBool TSize::operator!=(const TSize& aSize) const
/**
Compares two TSize for inequality.

For two TSize to be unequal, either their width or height values must be different.

@param aSize The TSize to be compared with this TSize. 

@return True, if the two TSize are unequal; false, otherwise.
*/
	{
	return(iWidth!=aSize.iWidth || iHeight!=aSize.iHeight);
	}




EXPORT_C TSize& TSize::operator-=(const TSize& aSize)
/**
TSize subtraction assignment operator.

The operator subtracts the specified TSize from this TSize, and assigns the 
result back to this TSize.

@param aSize The TSize to be subtracted. 

@return A reference to this TSize object.
*/
	{
	iWidth-=aSize.iWidth;iHeight-=aSize.iHeight;return(*this);
	}




EXPORT_C TSize& TSize::operator-=(const TPoint& aPoint)
/**
TPoint subtraction assignment operator.

The operator subtracts the specified point from this TSize, and assigns the 
result back to this TSize.

The operation proceeds by:

1. subtracting the point's x co-ordinate value from the width

2. subtracting the point's y co-ordinate value from the height.

@param aPoint The point to be subtracted.

@return A reference to this size object.
*/
	{
	iWidth-=aPoint.iX;iHeight-=aPoint.iY;return(*this);
	}




EXPORT_C TSize& TSize::operator+=(const TSize& aSize)
/**
TSize addition assignment operator.

The operator adds the specified TSize to this TSize, and assigns the result 
back to this TSize.

@param aSize The TSize to be added.

@return A reference to this size object.
*/
	{
	iWidth+=aSize.iWidth;iHeight+=aSize.iHeight;return(*this);
	}




EXPORT_C TSize& TSize::operator+=(const TPoint& aPoint)
/**
TPoint addition assignment operator.

The operator adds the specified point to this TSize, and assigns the result 
back to this TSize.

The operation proceeds by:

1. adding the point's x co-ordinate value to the width

2. adding the point's y co-ordinate value to the height.

@param aPoint The point to be added.

@return A reference to this size object.
*/
	{
	iWidth+=aPoint.iX;iHeight+=aPoint.iY;return(*this);
	}




EXPORT_C TSize TSize::operator-(const TSize& aSize) const
/**
TSize subtraction operator.

This operator subtracts the specified TSize from this TSize, and returns the 
resulting value.

@param aSize The TSize to be subtracted from this Tsize.

@return The result of the operation.
*/
	{
	TSize ret=* this; ret-= aSize ;return(ret);
	}




EXPORT_C TSize TSize::operator-(const TPoint& aPoint) const
/**
TPoint subtraction operator.

This operator subtracts the specified point from this TSize, and returns the 
resulting value.

The operation proceeds by:

1. subtracting the x co-ordinate value from the width

2. subtracting the y co-ordinate value from the height

@param aPoint The point to be subtracted.

@return The result of the operation.
*/
	{
	TSize ret=* this; ret-= aPoint ;return(ret);
	}




EXPORT_C TSize TSize::operator+(const TSize& aSize) const
/**
TSize addition operator.

This operator adds the specified TSize to this TSize, and returns the resulting 
value.

@param aSize The TSize to be added to this Tsize. 

@return The result of the operation.
*/
	{
	TSize ret=* this; ret+= aSize ;return(ret);
	}




EXPORT_C TSize TSize::operator+(const TPoint& aPoint) const
/**
TPoint addition operator.

This operator adds the specified point to this TSize, and returns the resulting 
value.

The operation proceeds by:

1. adding the x co-ordinate value to the width

2. adding the y co-ordinate value to the height

@param aPoint The point to be added to this TSize. 

@return The result of the operation.
*/
	{
	TSize ret=* this; ret+= aPoint ;return(ret);
	}




EXPORT_C TSize TSize::operator-() const
/**
Unary minus operator.

The operator returns the negation of this TSize.

@return The result of the operation.
*/
	{

	return TSize(-iWidth,-iHeight);
	}




EXPORT_C void TSize::SetSize(TInt aWidth,TInt aHeight)
/**
Sets the width and height.

@param aWidth The width value. 
@param aHeight The height value.
*/
	{
	iWidth=aWidth;iHeight=aHeight;
	}




EXPORT_C TPoint TSize::AsPoint() const
/**
Gets a TPoint object whose co-ordinates are the width and height of this TSize.

@return The co-ordinates of this TSize converted to a point.
*/
	{
	return(TPoint(iWidth,iHeight));
	}




EXPORT_C TRect::TRect()	: iTl(0,0),iBr(0,0)
/**
Constructs a default rectangle.

This initialises the co-ordinates of its top 
left and bottom right corners to (0,0).
*/
	{}
	
	
	
	
EXPORT_C TRect::TRect(TInt aAx,TInt aAy,TInt aBx,TInt aBy) : iTl(aAx,aAy),iBr(aBx,aBy)
/**
Constructs the rectangle, initialising its top left and bottom right hand corners 
with four TInt values.

@param aAx The horizontal co-ordinate of the left hand side of the rectangle. 
@param aAy The vertical co-ordinate of the top of the rectangle. 
@param aBx The horizontal co-ordinate of the right hand side of the rectangle. 
@param aBy The vertical co-ordinate of the bottom of the rectangle.
*/
	{}
	
	
	
	
EXPORT_C TRect::TRect(const TPoint& aPointA,const TPoint& aPointB) : iTl(aPointA),iBr(aPointB)
/**
Constructs the rectangle with two TPoints, corresponding to its top left and 
bottom right hand corners.

@param aPointA The co-ordinates of the rectangle's top left hand corner. 
@param aPointB The co-ordinates of the rectangle's bottom right hand corner.
*/
	{}
	
	
	
	
EXPORT_C TRect::TRect(const TPoint& aPoint,const TSize& aSize) : iTl(aPoint),iBr(aPoint+aSize)
/**
Constructs the rectangle with a TPoint for its top left corner, and a TSize 
for its width and height.

@param aPoint The rectangle's top left hand corner. 
@param aSize  The rectangle's width and height.
*/
	{}
	
	
	
	
EXPORT_C TRect::TRect(const TSize& aSize) : iTl(0,0), iBr(aSize.iWidth, aSize.iHeight)
/**
Constructs the rectangle with a TSize.

The co-ordinates of its top left hand corner are initialised to (0,0) and
its width and height are initialised to the values contained in the argument.

@param aSize The width and height of the rectangle.
*/
	{}
	
	
	

EXPORT_C void TRect::SetRect(TInt aTlX,TInt aTlY,TInt aBrX,TInt aBrY)
/**
Sets the rectangle's position using four TInts.
	
@param aTlX The horizontal co-ordinate of the left hand side of the rectangle. 
@param aTlY The vertical co-ordinate of the top of the rectangle. 
@param aBrX The horizontal co-ordinate of the right hand side of the rectangle. 
@param aBrY The vertical co-ordinate of the bottom of the rectangle.
*/
	{
	iTl.iX=aTlX;iTl.iY=aTlY;iBr.iX=aBrX;iBr.iY=aBrY;
	}
	
	
	

EXPORT_C void TRect::SetRect(const TPoint& aPointTL,const TPoint& aPointBR)
/**
Sets the rectangle's position using two TPoints.

@param aPointTL The co-ordinates of the rectangle's top left hand corner. 
@param aPointBR The co-ordinates of the rectangle's bottom right hand corner.
*/
	{
	iTl=aPointTL;iBr=aPointBR;
	}




EXPORT_C void TRect::SetRect(const TPoint& aTL,const TSize& aSize)
/**
Sets the rectangle's position using a TPoint and a TSize.
	
@param aTL    The co-ordinates of the rectangle's top left hand corner. 
@param aSize  The rectangle's width and height.
*/
	{
	iTl=aTL;iBr=aTL+aSize;
	}




EXPORT_C void TRect::Shrink(TInt aDx,TInt aDy)
/**
Shrinks a rectangle using specified horizontal and vertical offsets. 

The offset values are added to the co-ordinates of its top left hand corner, 
and the same values are subtracted from the co-ordinates of its bottom right 
hand corner. The co-ordinates of the centre of the rectangle remain unchanged. 
If either value is negative, the rectangle expands in the corresponding direction.

@param aDx The number of pixels by which to move the left and right hand sides 
           of the rectangle. A positive value reduces the width, a negative
           value increases it. 
@param aDy The number of pixels by which to move the top and bottom of the 
           rectangle. A positive value reduces the height, a negative value
           increases it.
*/
	{
	Adjust(aDx,aDy);
	}




EXPORT_C void TRect::Shrink(const TSize& aSize)
/**
Shrinks a rectangle using a specified TSize offset.

The rectangle shrinks by twice the value of the height and width specified 
in the TSize. The co-ordinates of the centre of the rectangle remain unchanged. 
If either value is negative, the rectangle expands in the
corresponding direction.

@param aSize The number of pixels by which to move the left and right hand 
             sides of the rectangle (by aSize.iWidth) and the top and bottom
             (by aSize.iHeight).
*/
	{
	Adjust(aSize.iWidth,aSize.iHeight);
	}




EXPORT_C void TRect::Resize(const TSize& aSize)
/**
Resizes a rectangle by adding a TSize offset.

The offset is added to the co-ordinates of its bottom right hand corner. If 
either value in the TSize is negative, the rectangle shrinks in the
corresponding direction. The co-ordinates of the rectangle's top left hand
corner are unaffected.

@param aSize The number of pixels by which to move the rectangle; the right 
             hand side by aSize.iWidth and the bottom by aSize.iHeight.
*/
	{
	iBr+=aSize;
	}




EXPORT_C void TRect::Resize(TInt aDx, TInt aDy)
/**
Resizes a rectangle by adding a horizontal and vertical offset.

The offset is added to the co-ordinates of its bottom right hand corner. If 
either value is negative, the rectangle shrinks in the corresponding direction.
The co-ordinates of the rectangle's top left hand corner are unaffected.

@param aDx The number of pixels by which to move the right hand side of the 
           rectangle. 
@param aDy The number of pixels by which to move the bottom of the rectangle.
*/
	{
	iBr.iX+=aDx;iBr.iY+=aDy;
	}




EXPORT_C TBool TRect::operator==(const TRect &aRect) const
/**
Compares two rectangles for equality.

For two rectangles to be equal, the co-ordinates of both their top left and 
bottom right hand corners must be equal.

@param aRect The rectangle to compare with this rectangle.

@return True, if the rectangles have the same co-ordinates; false, otherwise.
*/
	{

	return(iTl==aRect.iTl && iBr==aRect.iBr);
	}




EXPORT_C TBool TRect::operator!=(const TRect &aRect) const
/**
Compares two rectangles for inequality.

Two rectangles are unequal if any of their co-ordinates differ.

@param aRect The rectangle to compare with this rectangle.
@return True, if the rectangles do not have the same co-ordinates; false, if 
        all co-ordinates are equal.
*/
	{

	return(iTl!=aRect.iTl || iBr!=aRect.iBr);
	}




EXPORT_C void TRect::Move(const TPoint &aOffset)
/**
Moves the rectangle by adding a TPoint offset.

The offset is added to the co-ordinates of both its top left and bottom right 
hand corners. The size of the rectangle is unchanged.

@param aOffset The number of pixels to move the rectangle; horizontally by 
               aOffset.iX and vertically by aOffset.iY.
*/
	{

	iTl+=aOffset;
	iBr+=aOffset;
	}




EXPORT_C void TRect::Move(TInt aDx,TInt aDy)
/**
Moves the rectangle by adding an x, y offset.

The offset is added to the co-ordinates of both its top left and bottom right 
hand corners. The size of the rectangle is unchanged.

@param aDx The number of pixels to move the rectangle horizontally. If negative, 
           the rectangle moves leftwards. 
@param aDy The number of pixels to move the rectangle vertically. If negative, 
           the rectangle moves upwards.
*/
	{

	iTl.iX+=aDx;
	iTl.iY+=aDy;
	iBr.iX+=aDx;
	iBr.iY+=aDy;
	}




// private function, hence not exported
void TRect::Adjust(TInt aDx,TInt aDy)
//
// Adjust by a delta.
//
	{

	iTl.iX+=aDx;
	iTl.iY+=aDy;
	iBr.iX-=aDx;
	iBr.iY-=aDy;
	}




EXPORT_C void TRect::Grow(TInt aDx,TInt aDy)
//
// Grow by a delta.
//
/**
Grows the rectangle using the specified horizontal and vertical offsets.

The offset values are subtracted from the co-ordinates of its top left hand 
corner, and the same values are added to the co-ordinates of its bottom right 
hand corner. The co-ordinates of the centre of the rectangle remain unchanged. 
If either value is negative, the rectangle shrinks in the corresponding direction.

@param aDx The number of pixels by which to move the left and right hand sides 
           of the rectangle. A positive value increases the width, a negative
           value reduces it. 
@param aDy The number of pixels by which to move the top and bottom of the 
           rectangle. A positive value increases the height, a negative
           value reduces it.
*/
	{

	iTl.iX-=aDx;
	iTl.iY-=aDy;
	iBr.iX+=aDx;
	iBr.iY+=aDy;
	}




EXPORT_C void TRect::Grow(const TSize &aSize)
//
// Grow by a size.
//
/**
Grows a rectangle using the specified TSize offset.

The rectangle grows by twice the value of the height and width specified in 
the TSize. The co-ordinates of the centre of the rectangle remain unchanged. 
If either value is negative, the rectangle shrinks in the
corresponding direction.

@param aSize The number of pixels by which to move the left and right hand 
             sides of the rectangle (by aSize.iWidth) and the top and bottom
             (by aSize.iHeight).
*/
	{

	iTl-=aSize;
	iBr+=aSize;
	}




EXPORT_C void TRect::BoundingRect(const TRect &aRect)
//
// Union of this and aRect, a union is defined as the minimum rectangle that encloses
// both source rectangles
//
/** 
Gets the minimal rectangle which bounds both this rectangle and the specified 
rectangle, and assigns it to this rectangle.

@param aRect The rectangle to use with this rectangle to get the minimal bounding 
             rectangle.
*/
	{

	if (iTl.iX>aRect.iTl.iX)
		iTl.iX=aRect.iTl.iX;
	if (iTl.iY>aRect.iTl.iY)
		iTl.iY=aRect.iTl.iY;
	if (iBr.iX<aRect.iBr.iX)
		iBr.iX=aRect.iBr.iX;
	if (iBr.iY<aRect.iBr.iY)
		iBr.iY=aRect.iBr.iY;
	}




EXPORT_C TBool TRect::IsEmpty() const
//
// True if the rectangle is empty.
//
/**
Tests whether the rectangle is empty.

@return True, if empty; false, if not.
*/
	{

	return(iTl.iX>=iBr.iX || iTl.iY>=iBr.iY);
	}




EXPORT_C void TRect::Intersection(const TRect &aRect)
//
// Intersect this with aRect.
//
/**
Gets the area of intersection between this rectangle and the specified
rectangle, and assigns it to this rectangle.

It is usual to call TRect::Intersects() first to verify whether the two rectangles 
intersect. If the two rectangles do not intersect, then, on return, this rectangle 
is set to be empty.

@param aRect The rectangle to be used with this rectangle to get the area
             of intersection.
             
@see TRect::Intersects             
*/
	{

	if (iTl.iX<aRect.iTl.iX)
		iTl.iX=aRect.iTl.iX;
	if (iTl.iY<aRect.iTl.iY)
		iTl.iY=aRect.iTl.iY;
	if (iBr.iX>aRect.iBr.iX)
		iBr.iX=aRect.iBr.iX;
	if (iBr.iY>aRect.iBr.iY)
		iBr.iY=aRect.iBr.iY;
	}




EXPORT_C TBool TRect::Intersects(const TRect &aRect) const
//
// If aRect Intersects with this return True.
//
/**
Tests whether this rectangle overlaps with the specified rectangle.

Two rectangles overlap if any point is located within both rectangles. There 
is no intersection if two adjacent sides touch without overlapping, or if 
either rectangle is empty.

@param aRect The rectangle to compare with this rectangle for an intersection. 

@return True, if the two rectangles overlap; false, if there is no overlap.
*/
	{

	return(!(IsEmpty() || aRect.IsEmpty() || iBr.iX<=aRect.iTl.iX || iBr.iY<=aRect.iTl.iY || iTl.iX>=aRect.iBr.iX || iTl.iY>=aRect.iBr.iY));
	}




EXPORT_C void TRect::Normalize()
//
// Make the conditions top left bottom right true.
//
/**
Ensures that the rectangle's width and height have positive values.

For example, if the rectangle's co-ordinates are such that the top is below 
the bottom, or the right hand side is to the left of the left hand side, normalisation 
swaps the co-ordinates of the top and bottom or of the left and right.
*/
	{

	if (iTl.iX>iBr.iX)
		{
		TInt temp=iTl.iX;
		iTl.iX=iBr.iX;
		iBr.iX=temp;
		}
	if (iTl.iY>iBr.iY)
		{
		TInt temp=iTl.iY;
		iTl.iY=iBr.iY;
		iBr.iY=temp;
		}
	}




EXPORT_C TBool TRect::Contains(const TPoint &aPoint) const
/**
Tests whether a point is located within the rectangle.

Note that a point located on the top or left hand side of the rectangle is 
within the rectangle. A point located on the right hand side or bottom is 
considered to be outside the rectangle.

@param aPoint The point to be tested.

@return True, if the point is within the rectangle; false, otherwise.
*/
	{
	if (aPoint.iX<iTl.iX || aPoint.iX>=iBr.iX || aPoint.iY<iTl.iY || aPoint.iY>=iBr.iY)
		return(EFalse);
	return(ETrue);
	}




EXPORT_C TSize TRect::Size() const
/**
Gets the size of the rectangle.

@return The size of the rectangle.
*/
	{
	return((iBr-iTl).AsSize());
	}




EXPORT_C TInt TRect::Width() const
/**
Gets the width of the rectangle.

@return The width of the rectangle.
*/
	{
	return(iBr.iX-iTl.iX);
	}




EXPORT_C TInt TRect::Height() const
/**
Gets the height of the rectangle.

@return The height of the rectangle.
*/
	{
	return(iBr.iY-iTl.iY);
	}




EXPORT_C TBool TRect::IsNormalized() const
/**
Tests whether the rectangle is normalised.

A rectangle is normalised when its width and height are both zero or greater.

@return True, if normalised; false, if not.
*/
	{
	return((iBr.iX>=iTl.iX) && (iBr.iY>=iTl.iY));
	}




EXPORT_C TPoint TRect::Center() const
/**
Gets the point at the centre of the rectangle.

@return The point at the centre of the rectangle.
*/
	{
	return(TPoint((iTl.iX+iBr.iX)/2,(iTl.iY+iBr.iY)/2));
	}




EXPORT_C void TRect::SetSize(const TSize &aSize)
/**
Sets the size of the rectangle.

Only the co-ordinates of the bottom right hand corner of the rectangle are 
affected.

@param aSize The new width is aSize.iWidth. The new height is aSize.iHeight.
*/
	{
	iBr=iTl+aSize;
	}




EXPORT_C void TRect::SetWidth(TInt aWidth)
/**
Sets the width of the rectangle.

Only the position of the rectangle's right hand side is affected.

@param aWidth The new width of the rectangle.
*/
	{
	iBr.iX=iTl.iX+aWidth;
	}




EXPORT_C void TRect::SetHeight(TInt aHeight)
/**
Sets the height of the rectangle.

Only the position of the bottom of the rectangle is affected.

@param aHeight The new height of the rectangle.
*/
	{
	iBr.iY=iTl.iY+aHeight;
	}
