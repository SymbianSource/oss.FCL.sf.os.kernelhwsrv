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
// e32\euser\us_evnt.cpp
// 
//


#include "us_std.h"



/**
Gets the mouse or digitiser X-Y position.

@return The position.
*/
EXPORT_C TPoint TRawEvent::Pos() const
	{

	__ASSERT_DEBUG(iType==EPointerMove || 
                    iType==EPointerSwitchOn ||
					iType==EButton1Down ||
					iType==EButton1Up ||
					iType==EButton2Down ||
					iType==EButton2Up ||
					iType==EButton3Down ||
					iType==EButton3Up
					,Panic(ETEventNotMoveType));
	return(TPoint(iU.pos.x,iU.pos.y));
	}

/**
Gets the 3D pointing device Cartesian coordinates.

@return The position.
*/
EXPORT_C TPoint3D TRawEvent::Pos3D() const
	{

	__ASSERT_DEBUG(iType==EPointerMove || 
					iType==EPointer3DTiltAndMove ||
                    iType==EPointerSwitchOn ||
					iType==EButton1Down ||
					iType==EButton1Up ||
					iType==EButton2Down ||
					iType==EButton2Up ||
					iType==EButton3Down ||
					iType==EButton3Up
					,Panic(ETEventNotMoveType));
	TPoint3D p;
	p.iX=iU.pos.x;
	p.iY=iU.pos.y;
	p.iZ=iU.pos3D.z;
	return p;
	}

/** Gets the 3D pointing device polar coordinates and rotation.

	@return The polar coordinates and rotation.
 */
EXPORT_C TAngle3D TRawEvent::Tilt() const
	{
	__ASSERT_DEBUG(iType==EPointer3DTilt || 
					iType==EPointer3DTiltAndMove
					,Panic(ETEventNotMoveType));
	TAngle3D p;
	p.iPhi=iU.pos3D.phi;
	p.iTheta=iU.pos3D.theta;
	return p;
	}

/** Gets the rotation angle of 3D pointing device.
    
    @return The rotation angle of the 3D pointing device.
 */
EXPORT_C TInt TRawEvent::Rotation() const
	{
	__ASSERT_DEBUG(iType==EPointer3DRotation || 
					iType==EPointer3DTiltAndMove
					,Panic(ETEventNotMoveType));
	return (iU.pos3D.alpha);
	}

/**
Gets the scancode.

@return The scancode.
*/
EXPORT_C TInt TRawEvent::ScanCode() const
	{

	__ASSERT_DEBUG(iType==EKeyDown || iType==EKeyUp || iType==EKeyRepeat,Panic(ETEventNotKeyType));
	return(iU.key.scanCode);
	}




/**
Gets the repeat count.

@return The repeat count.
*/
EXPORT_C TInt TRawEvent::Repeats() const
	{
	__ASSERT_DEBUG(iType==EKeyRepeat,Panic(ETEventNotKeyRepeatType));
	return(iU.key.repeats);
	}




/**
Gets the modifiers.

@return The modifiers.

@panic EUSER 34 If the event does not represent a modifier key being pressed,
                i.e. if the event type is not TRawEvent::EUpdateModifiers
       
@see TRawEvent::EUpdateModifiers
*/
EXPORT_C TInt TRawEvent::Modifiers() const
	{

	__ASSERT_DEBUG(iType==EUpdateModifiers,Panic(ETEventNotUpdateModifiersType));
	return(iU.modifiers);
	}




/**
Sets the event up as a scancode event.

@param	aType	  The event type.
@param	aScanCode The scancode.
*/
EXPORT_C void TRawEvent::Set(TType aType,TInt aScanCode)
	{

	iType=(TUint8)aType;
	iU.key.scanCode=aScanCode;
	iU.key.repeats=0;
	BTraceContext8(BTrace::ERawEvent, BTrace::ESetEvent ,(TUint32)aType, (TUint32)aScanCode);
	iTicks=User::TickCount();
	}


/**
Sets the event up as a repeating scancode event.

@param	aType	  The event type.
@param	aScanCode The scancode.
@param	aRepeats  The repeat count.
*/
EXPORT_C void TRawEvent::SetRepeat(TType aType,TInt aScanCode,TInt aRepeats)
	{

	iType=(TUint8)aType;
	iU.key.scanCode=aScanCode;
	iU.key.repeats=aRepeats;
	BTraceContext12(BTrace::ERawEvent, BTrace::ESetEvent ,(TUint32)aType, (TUint32)aScanCode, (TUint32)aRepeats);
	iTicks=User::TickCount();
	}


/**
Sets the event up as a mouse/pen event.

@param	aType	The event type.
@param	aX		The X position.
@param	aY		The Y position.
*/
EXPORT_C void TRawEvent::Set(TType aType,TInt aX,TInt aY)
	{

	iType=(TUint8)aType;
	iU.pos.x=aX;
	iU.pos.y=aY;
	BTraceContext12(BTrace::ERawEvent, BTrace::ESetEvent ,(TUint32)aType, (TUint32)aX,(TUint32)aY);
	iTicks=User::TickCount();
	}

	
/**
Sets the event up as a 3D pointer linear move event.

@param	aType	The event type.
@param	aX		The X position.
@param	aY		The Y position.
@param	aZ		The Z position (defaults to 0 on 2D detection devices) .
*/
EXPORT_C void TRawEvent::Set(TType aType,TInt aX,TInt aY,TInt aZ)
	{

	iType=(TUint8)aType;
	iU.pos.x=aX;
	iU.pos.y=aY;
	iU.pos3D.z=aZ;
	//Form the Trace Data
	TUint32 traceData[2];
    traceData[0] =aY;
	traceData[1] =aZ;
    BTraceContextN(BTrace::ERawEvent, BTrace::ESetEvent ,(TUint32)aType, (TUint32)aX,traceData, sizeof(traceData));    
	iTicks=User::TickCount();
	}


/**
Sets up an event without specific parameters.

@param	aType The event type.
*/
EXPORT_C void TRawEvent::Set(TType aType)
	{

	iType=(TUint8)aType;
	BTraceContext4(BTrace::ERawEvent, BTrace::ESetEvent ,(TUint32)aType);
	iTicks=User::TickCount();
	}

/**
Sets the event up as a 3D pointer linear move and tilt and rotation change event.

@param	aType	The event type.
@param	aX		The X position (or TPhi polar coordinate).
@param	aY		The Y position (or Theta polar coordinate).
@param	aZ		The Z position (or rotation).
@param	aPhi	The Phi polar coordinate.
@param	aTheta	The Theta polar coordinate.
@param	aAlpha	The rotation angle.
 */
EXPORT_C void TRawEvent::Set(TType aType,TInt aX,TInt aY,TInt aZ,TInt aPhi,TInt aTheta,TInt aAlpha)
	{
    
	iType=(TUint8)aType;
	iU.pos.x=aX;
	iU.pos.y=aY;
	iU.pos3D.z=aZ;
	iU.pos3D.phi=aPhi;
	iU.pos3D.theta=aTheta;
	iU.pos3D.alpha=aAlpha;	
	//Form the Trace Data
	TUint32 traceData[5];
    traceData[0] =aY;
	traceData[1] =aZ;
	traceData[2] =aPhi;
	traceData[3] =aTheta;
	traceData[4] =aAlpha;
	BTraceContextN(BTrace::ERawEvent, BTrace::ESetEvent ,(TUint32)aType, (TUint32)aX,traceData, sizeof(traceData));	
	iTicks=User::TickCount();
	}

/**
Sets the event up as 3D pointer tilt change event.

@param	aType	The event type.
@param	aPhi	The Phi coordinate.
@param	aTheta	The Theta coordinate.
*/
EXPORT_C void TRawEvent::SetTilt(TType aType,TInt aPhi,TInt aTheta)
	{

	iType=(TUint8)aType;
	iU.pos3D.phi=aPhi;
	iU.pos3D.theta=aTheta;	
	BTraceContext12(BTrace::ERawEvent, BTrace::ESetTiltEvent,(TUint32)aType, (TUint32)aPhi,(TUint32)aTheta);
	iTicks=User::TickCount();
	}

/**
Sets the event up as 3D pointer rotation event.

@param	aType	The event type.
@param	aAlpha	The rotation angle.
*/
EXPORT_C void TRawEvent::SetRotation(TType aType,TInt aAlpha)
	{

	iType=(TUint8)aType;
	iU.pos3D.alpha=aAlpha;
	BTraceContext8(BTrace::ERawEvent, BTrace::ESetRotationtEvent,(TUint32)aType, (TUint32)aAlpha);
	iTicks=User::TickCount();
	}


/**
Sets the event up as a 3D pointer linear move event with pointer number 
@param	aType	The event type.
@param	aX		The X position.
@param	aY		The Y position.
@param	aZ		The Z position .
@param  aPointerNumber   The pointer number for the event 
*/

EXPORT_C void TRawEvent::Set (TType aType, TInt aX, TInt aY, TInt aZ, TUint8 aPointerNumber)
	{

	iType=(TUint8)aType;
	iU.pos3D.x=aX;
	iU.pos3D.y=aY;
	iU.pos3D.z=aZ;
	iPointerNumber=aPointerNumber;		
	//Form the Trace Data
	TUint32 traceData[3];
    traceData[0] =aY;
	traceData[1] =aZ;
	traceData[2] =aPointerNumber;
    BTraceContextN(BTrace::ERawEvent, BTrace::ESetEvent ,(TUint32)aType, (TUint32)aX,traceData, sizeof(traceData));    	
	iTicks=User::TickCount();
	}

