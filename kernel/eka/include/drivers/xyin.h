// Copyright (c) 1998-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\xyin.h
// Generic digitiser driver header
//
//

/**
 @file
 @internalComponent
*/

#ifndef __M32XYIN_H__
#define __M32XYIN_H__
#include <kernel/kpower.h>
#include <platform.h>
#include <e32hal.h>


#ifdef _DEBUG
//#define __DIGITISER_DEBUG1__
//#define __DIGITISER_DEBUG2__
#endif
#ifdef __DIGITISER_DEBUG1__
#define __KTRACE_XY1(s) s;
#else
#define __KTRACE_XY1(s)
#endif
#ifdef __DIGITISER_DEBUG2__
#define __KTRACE_XY2(s) s;
#else
#define __KTRACE_XY2(s)
#endif

/**
@internalComponent
*/
const TInt KMaxXYSamples=4;

/**
@publishedPartner
@released
*/
struct SDigitiserConfig
	{
	TInt iPenDownDiscard;	// number of samples to discard on entering the detection volume (area if 2 dimensional)
	TInt iPenUpDiscard;		// number of samples to discard on leaving the detection volume (area if 2 dimensional)
	TInt iDriveXRise;		// number of milliseconds to wait when driving horizontal edges
	TInt iDriveYRise;		// number of milliseconds to wait when driving vertical edges
	TInt iMinX;				// minimum valid X value
	TInt iMaxX;				// maximum valid X value
	TInt iSpreadX;			// maximum valid X spread
	TInt iMinY;				// minimum valid Y value
	TInt iMaxY;				// maximum valid Y value
	TInt iSpreadY;			// maximum valid Y spread
	TInt iMaxJumpX;			// maximum X movement per sample (pixels)
	TInt iMaxJumpY;			// maximum Y movement per sample (pixels)
	TInt iAccThresholdX;	// accumulated offset in pixels to cause movement in X direction
	TInt iAccThresholdY;	// accumulated offset in pixels to cause movement in Y direction
	TInt iNumXYSamples;		// number of samples to average
	TBool iDisregardMinMax;	// TRUE if we want to disregard minimum and maximum
	};

/**
@publishedPartner
@prototype
*/
struct SDigitiserConfigV01
	{
	SDigitiserConfig i2dConfig;
	TInt iMinZ;				// minimum valid Z value (distance to screen): 0 when lighlty touching (no pressure applied)
	TInt iMaxZ;				// maximum valid Z value: positive for distance to screen, negative for pressure
	TInt iSpreadZ;			// maximum valid Z spread (in distance to screen units)
	TInt iMaxJumpZ;			// maximum Z movement per sample (in distance to screen units)
	TInt iAccThresholdX;	// accumulated offset in distance to screen units to cause movement in Z direction
	};

/**
@publishedPartner
@released
*/
NONSHARABLE_CLASS(DDigitiser) : public DPowerHandler
	{
public:
/**
@internalComponent
*/
	enum TState
		{
		EIdle=0,			// waiting for pen to go down
		EDiscardOnPenDown,	// discarding just after pen down
		EBufferFilling,		// buffer filling with samples
		EBufferFull,		// delay line is now full
		EPenDown,			// pen-down event has been delivered
		};
public:
	// initialisation
	static DDigitiser* New();
	DDigitiser();
	TInt Create();
	virtual TInt DoCreate()=0;
public:
	// signals from hardware-dependent code
	void RawSampleValid();
	void PenUp();
public:
	// signals to hardware-dependent code
	virtual void WaitForPenDown()=0;
	virtual void WaitForPenUp()=0;
	virtual void WaitForPenUpDebounce()=0;
	virtual void DigitiserOn()=0;
	virtual void DigitiserOff()=0;
public:
	// machine-configuration related things
	virtual TInt DigitiserToScreen(const TPoint& aDigitiserPoint, TPoint& aScreenPoint)=0;
	virtual void ScreenToDigitiser(TInt& aX, TInt& aY)=0;
	virtual TInt SetXYInputCalibration(const TDigitizerCalibration& aCalibration)=0;
	virtual TInt CalibrationPoints(TDigitizerCalibration& aCalibration)=0;
	virtual TInt SaveXYInputCalibration()=0;
	virtual TInt RestoreXYInputCalibration(TDigitizerCalibrationType aType)=0;
	virtual void DigitiserInfo(TDigitiserInfoV01& aInfo)=0;
public:
	// Generic stuff
/**
@internalComponent
*/
	void ProcessRawSample();
/**
@internalComponent
*/
	void ProcessPenUp();
/**
@internalComponent
*/
	TBool SamplesToPoint(TPoint& aPoint);
/**
@internalComponent
*/
	TInt DelayAndConvertSample(const TPoint& aSample, TPoint& aScreenPoint);
/**
@internalComponent
*/
	void IssuePenDownEvent();
/**
@internalComponent
*/
	void IssuePenUpEvent();
	void IssuePenMoveEvent(const TPoint& aPoint);
	virtual void FilterPenMove(const TPoint& aPoint)=0;
	virtual void ResetPenMoveFilter()=0;
/**
@internalComponent
*/
	virtual TInt HalFunction(TInt aFunction, TAny* a1, TAny* a2);
/**
@internalComponent
*/
 	void HandleMsg(TMessageBase* aMsg);
public:
	TDfcQue* iDfcQ;
 	TMessageQue iMsgQ;
	TDfc iSampleDfc;				// called when a raw sample is available
	TDfc iPenUpDfc;					// called when the pen goes up
	TInt iX[KMaxXYSamples];			// raw X samples from hardware
	TInt iY[KMaxXYSamples];			// raw Y samples from hardware
	SDigitiserConfig iCfg;			// configuration
	TInt iBufferIndex;				// delay line index
	TPoint* iBuffer;				// delay line for samples
	TPoint iLastPos;				// last pen position
	TState iState;
	TInt iCount;
	TUint8 iPointerOn;
	TInt iOrientation;	 			// HALData::TDigitizerOrientation
	};



#endif
