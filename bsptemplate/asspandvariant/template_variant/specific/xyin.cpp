// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// template\Template_Variant\Specific\xyin.cpp
// Implementation of a digitiser (touch-screen) driver. 
// This code assumes that an interrupt is generated on pen-down and pen-up events.
// This file is part of the Template Base port
// We use this driver to exemplify the usage of Resource Management for shared resources, Peripheral "Sleep"
// and detection and notification of Wakeup events
// 
//


#include <assp.h>
#include <template_assp.h>
#include <videodriver.h>
#include "mconf.h"
#include <drivers/xyin.h>
#include "template_power.h"




//
// TO DO: (mandatory)
//
// Define the following constants that describe the digitiser position & dimensions
// This is only example code... you need to modify it for your hardware

// digitiser origin & size in pixels
const TUint	KConfigXyOffsetX	= 0;		// digitiser origin - same as display area
const TUint	KConfigXyOffsetY	= 0;
const TUint	KConfigXyWidth		= 640;		// 640 pixels per line
const TUint	KConfigXyHeight		= 480;		// 480 lines per panel

// digitiser dimensions in digitiser co-ordinates
const TInt		KConfigXyBitsX			= 12;
const TInt		KConfigXyBitsY			= 12;
const TInt		KConfigXySpreadX		= 1 << KConfigXyBitsX;		// maximum valid X spread
const TInt		KConfigXySpreadY		= 1 << KConfigXyBitsY;		// maximum valid Y spread
const TInt		KConfigXyMinX			= 0;						// minimum valid X value
const TInt		KConfigXyMinY			= 0;						// minimum valid Y value
const TInt		KConfigXyMaxX			= KConfigXySpreadX - 1;		// maximum valid X value
const TInt		KConfigXyMaxY			= KConfigXySpreadY - 1;		// maximum valid Y value


// Define a 2x2 matrix and two constants Tx and Ty to convert digitiser co-ordinates 
// to pixels such that
//
// (X<<16 Y<<16)	=	(x y)	x	(R11 R12)	+	(Tx Ty)
//									(R21 R22)			
// or : 
//
// X = (x*R11 + y*R21 + TX) >> 16;
// Y = (x*R12 + y*R22 + TY) >> 16;

//
// where x,y are digitiser coordinates, Tx,Ty are constant offsets and X,Y are screen 
// coordinates. Left shifting by 16 bits is used so as not to lose precision.
//
// These are default values to be used before calibration has taken place
// These are best set by observation.
// The example values given below are for a digitiser whose origin is at bottom left
// (the screen origin is at top left)
const TInt		KConfigXyR11		= (KConfigXyWidth << 16) / KConfigXySpreadX;		// 10240
const TInt		KConfigXyR12		= 0;
const TInt		KConfigXyR21		= 0;
const TInt		KConfigXyR22		= - ((KConfigXyHeight << 16) / KConfigXySpreadY);	// -7680
const TInt		KConfigXyTx			= 0;
const TInt		KConfigXyTy			= (KConfigXyHeight << 16) / KConfigXySpreadY;

//
// TO DO: (optional)
//
// Define the following constants that describe the digitiser behaviour
// This is only example code... you need to modify it for your hardware

// After taking a sample, wait for the specified number of nano-kernel ticks (normally 1 ms) 
// before taking the next sample
const TInt		KInterSampleTime	= 1;	

// After a group of samples has been processed by the DDigitiser::ProcessRawSample() DFC,
// wait for the specified number of nano-kernel ticks before taking the next sample
const TInt		KInterGroupTime		= 1;

// After a pen-down interrupt, 
// wait for the specified number of nano-kernel ticks before taking the next sample
const TInt		KPenDownDelayTime	= 2;

// If powering up the device with the pen down, 
// wait for the specified number of nano-kernel ticks before taking the next sample
const TInt		KPenUpPollTime		= 30;

// After a pen-up interrupt, 
// wait for the specified number of nano-kernel ticks before calling PenUp()
const TInt		KPenUpDebounceTime	= 10;

// number of samples to discard on pen-down
const TInt		KConfigXyPenDownDiscard	= 1;		

// number of samples to discard on pen-up
const TInt		KConfigXyPenUpDiscard	= 1;		

// offset in pixels to cause movement in X direction
const TInt		KConfigXyAccThresholdX	= 12;		

// offset in pixels to cause movement in Y direction
const TInt		KConfigXyAccThresholdY	= 12;		

// number of samples to average - MUST be <= KMaxXYSamples
const TInt		KConfigXyNumXYSamples	= 2;		

// disregard extremal values in each 4-sample group
const TBool		KConfigXyDisregardMinMax= EFalse;	



// obsolete constants :
const TInt		KConfigXyDriveXRise		= 0;		
const TInt		KConfigXyDriveYRise		= 0;		
const TInt		KConfigXyMaxJumpX		= 0;		
const TInt		KConfigXyMaxJumpY		= 0;		



/******************************************************
 * Main Digitiser Class
 ******************************************************/

//
// TO DO: (optional)
//
// Add any private functions and data you require
//
NONSHARABLE_CLASS(DTemplateDigitiser) : public DDigitiser
	{
public:
	enum TState
		{
		E_HW_PowerUp,
		E_HW_PenUpDebounce,
		E_HW_CollectSample
		};

public:
	// from DDigitiser - initialisation
	DTemplateDigitiser();
	virtual TInt DoCreate();
	void SetDefaultConfig();

	// from DDigitiser - signals to hardware-dependent code
	virtual void WaitForPenDown();
	virtual void WaitForPenUp();
	virtual void WaitForPenUpDebounce();
	virtual void DigitiserOn();
	virtual void DigitiserOff();
	virtual void FilterPenMove(const TPoint& aPoint);
	virtual void ResetPenMoveFilter();

	// from DDigitiser - machine-configuration related things
	virtual TInt DigitiserToScreen(const TPoint& aDigitiserPoint, TPoint& aScreenPoint);
	virtual void ScreenToDigitiser(TInt& aX, TInt& aY);
	virtual TInt SetXYInputCalibration(const TDigitizerCalibration& aCalibration);
	virtual TInt CalibrationPoints(TDigitizerCalibration& aCalibration);
	virtual TInt SaveXYInputCalibration();
	virtual TInt RestoreXYInputCalibration(TDigitizerCalibrationType aType);
	virtual void DigitiserInfo(TDigitiserInfoV01& aInfo);

	// from DPowerHandler
	virtual void PowerDown(TPowerState);
	virtual void PowerUp();

public:
	// implementation
	void TakeSample();
	void PenInterrupt();
	void DigitiserPowerUp();
	void PowerUpDfc();

public:
	NTimer iTimer;
	NTimer iTimerInt;
	TDfc iTakeReadingDfc;
	TDfc iPowerDownDfc;
	TDfc iPowerUpDfc;
	TInt iSamplesCount;
	TState iState;
	TUint8 iPoweringDown;

	TSize iScreenSize;
	TActualMachineConfig& iMachineConfig;
	};

/******************************************************
 * Digitiser main code
 ******************************************************/
/**
Sample timer callback
Schedules a DFC to take a sample

@param aPtr	a pointer to DTemplateDigitiser
*/
LOCAL_C void timerExpired(TAny* aPtr)
	{
	DTemplateDigitiser* pD=(DTemplateDigitiser*)aPtr;
	__KTRACE_OPT(KHARDWARE,Kern::Printf("T"));
	pD->iTakeReadingDfc.Add();
	}

/**
Debounce timer callback 
schedules a DFC to process a pen-down interrupt

@param aPtr	a pointer to DTemplateDigitiser
*/
LOCAL_C void timerIntExpired(TAny* aPtr)
	{
	DTemplateDigitiser* pD=(DTemplateDigitiser*)aPtr;
	__KTRACE_OPT(KHARDWARE,Kern::Printf("TI"));
	// clear xy interrupt -> re-triggers the interrupt mechanism to catch the next IRQ

	// TO DO: (mandatory)
	// Write to the appropriate hardware register to clear the digitiser interrupt
	//

	pD->iTakeReadingDfc.Add();
	}

/**
Pen-up/down interrupt handler

@param aPtr	a pointer to DTemplateDigitiser
*/
LOCAL_C void penInterrupt(TAny* aPtr)
	{
	DTemplateDigitiser* pD=(DTemplateDigitiser*)aPtr;
	pD->PenInterrupt();
	}

/**
DFC for taking a sample

@param aPtr	a pointer to DTemplateDigitiser
*/
LOCAL_C void takeReading(TAny* aPtr)
	{
	DTemplateDigitiser* pD=(DTemplateDigitiser*)aPtr;
	pD->TakeSample();
	}

/**
DFC for powering down the device

@param aPtr	a pointer to DTemplateDigitiser
*/
LOCAL_C void powerDownDfc(TAny* aPtr)
	{
	DTemplateDigitiser* pD=(DTemplateDigitiser*)aPtr;
	pD->DigitiserOff();
	}

/**
DFC for powering up the device

@param aPtr	a pointer to DTemplateDigitiser
*/
LOCAL_C void powerUpDfc(TAny* aPtr)
	{
	DTemplateDigitiser* pD=(DTemplateDigitiser*)aPtr;
	pD->PowerUpDfc();
	}

/**
Creates a new instance of DDigitiser. 
Called by extension entry point (PIL) to create a DDigitiser-derived object.

@return	a pointer to a DTemplateDigitiser object
*/
DDigitiser* DDigitiser::New()
	{
	return new DTemplateDigitiser;
	}

/**
Default constructor
*/
DTemplateDigitiser::DTemplateDigitiser() :
		iTimer(timerExpired,this),
		iTimerInt(timerIntExpired,this),
		iTakeReadingDfc(takeReading,this,5),
		iPowerDownDfc(powerDownDfc,this,5),
		iPowerUpDfc(powerUpDfc,this,5),
		iMachineConfig(TheActualMachineConfig())
	{
	iDfcQ = Kern::DfcQue0();
	iTakeReadingDfc.SetDfcQ(iDfcQ);
	iPowerDownDfc.SetDfcQ(iDfcQ);
	iPowerUpDfc.SetDfcQ(iDfcQ);
	}

/**
Perform hardware-dependent initialisation

Called by platform independent layer
*/
TInt DTemplateDigitiser::DoCreate()
	{
	__KTRACE_OPT(KEXTENSION,Kern::Printf("DTemplateDigitiser::DoCreate"));

	if (Kern::ColdStart())
		{
		__KTRACE_OPT(KEXTENSION,Kern::Printf("Resetting digitiser calibration"));
		
		// Emergency digitiser calibration values
		iMachineConfig.iCalibration.iR11 = KConfigXyR11;
		iMachineConfig.iCalibration.iR12 = KConfigXyR12;
		iMachineConfig.iCalibration.iR21 = KConfigXyR21;
		iMachineConfig.iCalibration.iR22 = KConfigXyR22;
		iMachineConfig.iCalibration.iTx = KConfigXyTx;
		iMachineConfig.iCalibration.iTy = KConfigXyTy;
		}

	// register power handler
	Add();
	DigitiserPowerUp();

	// bind to the pen-up/down interrupt
	TInt r=Interrupt::Bind(KIntIdDigitiser, penInterrupt, this);
	if (r!=KErrNone)
		return r;

	// set up the default configuration
	SetDefaultConfig();

	return r;
	}

/**
Initialise the DDigitiser::iCfg structure
*/
void DTemplateDigitiser::SetDefaultConfig()
	{
	iCfg.iPenDownDiscard = KConfigXyPenDownDiscard;		// number of samples to discard on pen-down
	iCfg.iPenUpDiscard = KConfigXyPenUpDiscard;			// number of samples to discard on pen-up
	iCfg.iDriveXRise = KConfigXyDriveXRise;				// number of milliseconds to wait when driving horizontal edges
	iCfg.iDriveYRise = KConfigXyDriveYRise;				// number of milliseconds to wait when driving vertical edges
	iCfg.iMinX = KConfigXyMinX;							// minimum valid X value
	iCfg.iMaxX = KConfigXyMaxX;							// maximum valid X value
	iCfg.iSpreadX = KConfigXySpreadX;					// maximum valid X spread
	iCfg.iMinY = KConfigXyMinY;							// minimum valid Y value
	iCfg.iMaxY = KConfigXyMaxY;							// maximum valid Y value
	iCfg.iSpreadY = KConfigXySpreadY;					// maximum valid Y spread
	iCfg.iMaxJumpX = KConfigXyMaxJumpX;					// maximum X movement per sample (pixels)
	iCfg.iMaxJumpY = KConfigXyMaxJumpY;					// maximum Y movement per sample (pixels)
	iCfg.iAccThresholdX = KConfigXyAccThresholdX;		// offset in pixels to cause movement in X direction
	iCfg.iAccThresholdY = KConfigXyAccThresholdY;		// offset in pixels to cause movement in Y direction
	iCfg.iNumXYSamples = KConfigXyNumXYSamples;			// number of samples to average
	iCfg.iDisregardMinMax = KConfigXyDisregardMinMax;	// disregard extremal values in each 4-sample group
	}

/**
Takes a sample from the digitiser.
Called in the context of a DFC thread.
*/
void DTemplateDigitiser::TakeSample()
	{
	TTemplatePowerController::WakeupEvent();	// notify of pendown (wakeup event) and let the power controller sort
												// out if it needs propagation

	TBool penDown = EFalse;

	// TO DO: (mandatory)
	// Read from appropriate hardware register to determine whether digitiser panel is being touched
	// Set penDown to ETrue if touched or EFalse if not touched.
	//

	__KTRACE_OPT(KHARDWARE,Kern::Printf("TS: S%d PD%d Sp%d", (TInt)iState, penDown?1:0, iSamplesCount));

	if (iState==E_HW_PowerUp)
		{
		// waiting for pen to go up after switch on due to pen down or through the HAL
		// coverity[dead_error_condition]
		// The next line should be reachable when this template file is edited for use
		if (!penDown)							// pen has gone up -> transition to new state
			{
			iState=E_HW_CollectSample;
			iSamplesCount=0;						// reset sample buffer

			// TO DO: (mandatory)
			// Write to the appropriate hardware register to clear the digitiser interrupt
			//

			// TO DO: (mandatory)
			// Write to the appropriate hardware register(s) to allow the hardware 
			// to detect when the digitizer panel is touched
			//

			Interrupt::Enable(KIntIdDigitiser);		// enable pen-down interrupt

 			// TO DO: (mandatory)
			// Write to the appropriate hardware register to enable the digitiser interrupt
			//

			}
		else									// pen is still down, wait a bit longer in this state
			{
			iTimer.OneShot(KPenUpPollTime);
			}
		return;
		}

	if (!penDown)
		{
		if (iState==E_HW_PenUpDebounce)
			{
			iState=E_HW_CollectSample;	// back to initial state, no samples collected
			iSamplesCount=0;			// reset sample buffer
			// Need to lock the kernel to simulate ISR context and thus defer preemption, 
			// since PenUp() expects an ISR context and calls TDfc::Add().
			NKern::Lock();
			PenUp();					// adds DFC
			NKern::Unlock();
			}
		else							// iState=E_HW_CollectSample
			{
			iState=E_HW_PenUpDebounce;
			iTimer.OneShot(KPenUpDebounceTime);		// wait a bit to make sure pen still up
			}
		return;
		}
	else if (iState==E_HW_PenUpDebounce)	// pen down
		{
		// false alarm - pen is down again
		iState=E_HW_CollectSample;		// take a new set of samples
		iSamplesCount=0;				// reset sample buffer
		}
	// default: pen down and iState=E_HW_CollectSample

	// TO DO: (mandatory)
	// Read from appropriate hardware register to get the current digitiser coordinates 
	// of the point that is being touched. Set aResults accordingly.
	// This is only example code... you need to modify it for your hardware
	//
	TPoint aResults;

	// X axis
	iX[iSamplesCount] = aResults.iX;
	// Y axis
	iY[iSamplesCount] = aResults.iY;

	__KTRACE_OPT(KHARDWARE,Kern::Printf("Raw: X=%d Y=%d",iX[iSamplesCount],iY[iSamplesCount]));

	// Put the hardware back into pen-detect mode

	// TO DO: (mandatory)
	// Write to the appropriate hardware register(s) to allow the hardware 
	// to detect when the digitizer panel is touched
	//

	// count samples collected - if it's less than minimum,
	// schedule the reading of another sample
	if (++iSamplesCount < iCfg.iNumXYSamples)	// iX[] and iY[] are 4 levels deep in xyin.h...
		{
		if(KInterSampleTime > 0)				// need to check this config param as it might be zero!
			iTimer.OneShot(KInterSampleTime);	// haven't got a complete group yet, so queue timer to sample again
		else
			iTakeReadingDfc.Enque();
		return;
		}

	// Have a complete group of samples so pass up to processing layer (PIL)

	// Need to lock the kernel to simulate ISR context and thus defer preemption, 
	// since RawSampleValid() expects an ISR context and calls TDfc::Add().
	NKern::Lock();			
	RawSampleValid();		// adds DFC
	NKern::Unlock();
	}

/**
Request for an interrupt to be generated when the pen is next down
Called by PIL at startup or when pen leaves digitiser after pen-up event issued
*/
void DTemplateDigitiser::WaitForPenDown()
	{
	// Called at startup or when pen leaves digitiser after pen-up event issued
	__KTRACE_OPT(KHARDWARE,Kern::Printf("WD: PowerDownMask %x",iPoweringDown));
	if (iPoweringDown)
		{
		// powering down

		// TO DO: (mandatory)
		// Write to the appropriate hardware register(s) to allow the hardware 
		// to detect when the digitizer panel is touched and wakes up the system if in standby
		//

		//
		// TO DO: (optional)
		//
		// Relinquish request on power resources
		// This will place the peripheral hardware in a low power "Sleep" mode which is Interrupt detection capable
		// EXAMPLE ONLY
		//
		TemplateResourceManager* aManager = TTemplatePowerController::ResourceManager();
		aManager -> ModifyToLevel(TemplateResourceManager::AsynchMlResourceUsedByXOnly, 50 /* a percentage */);
		aManager -> SharedBResource1() -> Release();

		iPoweringDown = EFalse;
		PowerDownDone();
		}
	else
		{

		// TO DO: (mandatory)
		// Write to the appropriate hardware register to clear the digitiser interrupt
		//

		// TO DO: (mandatory)
		// Write to the appropriate hardware register(s) to allow the hardware 
		// to detect when the digitizer panel is touched
		//

		if ((iTimer.iState == NTimer::EIdle) && (iTimerInt.iState == NTimer::EIdle))
			Interrupt::Enable(KIntIdDigitiser);		// enable pen-down interrupt
		}
	}

/**
Called by PIL after it has processed a group of raw samples while pen is down.
Used to indicate that the iX, iY buffers may be re-used
*/
void DTemplateDigitiser::WaitForPenUp()
	{
	__KTRACE_OPT(KHARDWARE,Kern::Printf("WU"));
	iState = E_HW_CollectSample;
	iSamplesCount = 0;					// reset sample buffer
	if(KInterGroupTime > 0)				// need to check this config param as it might be zero!
		iTimer.OneShot(KInterGroupTime);
	else
		iTakeReadingDfc.Enque();
	}

/**
Called by PIL if the group of samples collected is not good enough
Used to indicate that the iX, iY buffers may be re-used
*/
void DTemplateDigitiser::WaitForPenUpDebounce()
	{
	__KTRACE_OPT(KHARDWARE,Kern::Printf("WUDB"));
	iState = E_HW_CollectSample;
	iSamplesCount = 0;			// reset sample buffer
	if(KInterGroupTime > 0)					// need to check this config param as it might be zero!
		iTimer.OneShot(KInterGroupTime);
	else
		iTakeReadingDfc.Enque();
	}

/**
Pen up/down interrupt service routine (ISR)
*/
void DTemplateDigitiser::PenInterrupt()
    {
	__KTRACE_OPT(KHARDWARE,Kern::Printf("I"));

	
	Interrupt::Clear(KIntIdDigitiser);	// should already have been cleared

	// TO DO: (mandatory)
	// Read from appropriate hardware register to determine whether digitiser panel is being touched
	// Set penDown to ETrue if touched or EFalse if not touched.
	// This is only example code... you need to modify it for your hardware
	TBool penDown = EFalse;

	// coverity[dead_error_condition]
	if(!penDown)					// pen up
		{

		// TO DO: (mandatory)
		// Write to the appropriate hardware register to clear the digitiser interrupt
		//

		// TO DO: (mandatory)
		// Write to the appropriate hardware register(s) to allow the hardware 
		// to detect when the digitizer panel is touched
		//

		return;						// ... and exit!
		}

	Interrupt::Disable(KIntIdDigitiser);		// do NOT disable the capability to generate interrupts at the source

    // Add the timing of pen interrupts as entropy data for the RNG
	Interrupt::AddTimingEntropy();

	if (KPenDownDelayTime>0)					// need to check this config param as it might be zero!
		iTimerInt.OneShot(KPenDownDelayTime);	// start a debounce timer which will queue a DFC to process the interrupt
	else
		{

		// TO DO: (mandatory)
		// Write to the appropriate hardware register to clear the digitiser interrupt
		// This will re-trigger the interrupt mechanism to catch the next interrupt...
		//

		iTakeReadingDfc.Add();
		}
	}

/**
DPowerHandler pure virtual
*/
void DTemplateDigitiser::PowerUp()
	{
	iPowerUpDfc.Enque();			// queue a DFC in this driver's context
	}

/**
Called by power up DFC
*/
void DTemplateDigitiser::PowerUpDfc()
	{
	__KTRACE_OPT(KPOWER, Kern::Printf("DTemplateDigitiser::PowerUpDfc()"));
	DigitiserOn();
	PowerUpDone();			// must be called from a different thread than PowerUp()
	}

/**
Turn the digitiser on
May be called as a result of a power transition or from the HAL
If called from HAL, then the digitiser may be already be on (iPointerOn == ETrue)
*/
void DTemplateDigitiser::DigitiserOn()
	{
	__KTRACE_OPT(KPOWER,Kern::Printf("DTemplateDigitiser::DigitiserOn() iPointerOn=%d", iPointerOn));

	if (!iPointerOn)				// may have been powered up already
		DigitiserPowerUp();
	}

/**
Power-up the digitiser. Assumes digitiser is off.
*/
void DTemplateDigitiser::DigitiserPowerUp()
	{
	__KTRACE_OPT(KPOWER, Kern::Printf("DigitiserPowerUp"));
	iPointerOn = ETrue;		// now turned on

	// TO DO: (mandatory)
	// Write to the appropriate hardware register to clear the digitiser interrupt
	//

	//
	// TO DO: (optional)
	//
	// Reassert request on power resources
	// This will move the peripheral hardware out of low power "Sleep" mode back to fully operational
	// EXAMPLE ONLY
	//
	TemplateResourceManager* aManager = TTemplatePowerController::ResourceManager();
	aManager -> ModifyToLevel(TemplateResourceManager::AsynchMlResourceUsedByXOnly, 100 /* a percentage */);
	aManager -> SharedBResource1() -> Use();

	// TO DO: (mandatory)
	// Write to the appropriate hardware register(s) to allow the hardware 
	// to detect when the digitizer panel is touched
	//

	iState = E_HW_PowerUp;	// so we wait for pen up if necessary
	iTakeReadingDfc.Enque();
	}

/**
DPowerHandler pure virtual

@param aPowerState the current power state
*/
void DTemplateDigitiser::PowerDown(TPowerState /*aPowerState*/)
	{
	iPoweringDown = ETrue;
	iPowerDownDfc.Enque();						// queue a DFC in this driver's context
	}

/**
Turn the digitiser off
May be called as a result of a power transition or from the HAL
If called from Power Manager, then the digitiser may be already be off (iPointerOn == EFalse)
if the platform is in silent running mode
*/
void DTemplateDigitiser::DigitiserOff()
	{
	__KTRACE_OPT(KPOWER,Kern::Printf("DTemplateDigitiser::DigitiserOff() iPointerOn=%d", iPointerOn));
	if (iPointerOn)		// can have been powered down from HAL
		{
		iPointerOn = EFalse;
		Interrupt::Disable(KIntIdDigitiser);

		// TO DO: (mandatory)
		// Write to the appropriate hardware register to disable the digitiser interrupt
		//

		iTimer.Cancel();
		iTimerInt.Cancel();
		iTakeReadingDfc.Cancel();
		if (iState != E_HW_CollectSample)
			{
			// Need to lock the kernel to simulate ISR context and thus defer preemption, 
			// since PenUp() expects an ISR context and calls TDfc::Add().
			NKern::Lock();
			PenUp();		// adds DFC (will call WaitForPenDown)
			NKern::Unlock();
			}
		else
			{

			// TO DO: (mandatory)
			// Write to the appropriate hardware register(s) to allow the hardware 
			// to detect when the digitizer panel is touched and wakes up the system if in standby
			//

			//
			// TO DO: (optional)
			//
			// Relinquish request on power resources as we are being powered down
			// This will place the peripheral hardware in a low power "Sleep" mode which is Interrupt detection capable
			// EXAMPLE ONLY
			//
			TemplateResourceManager* aManager = TTemplatePowerController::ResourceManager();
			aManager -> ModifyToLevel(TemplateResourceManager::AsynchMlResourceUsedByXOnly, 0 /* a percentage */);
			aManager -> SharedBResource1() -> Release();

			if (iPoweringDown)			// came here through PowerDown
				{
				iPoweringDown = EFalse;
				PowerDownDone();
				}
			}
		}
	else	// already powered down (by HAL)
		{
			if (iPoweringDown)			// came here through PowerDown
				{
				iPoweringDown = EFalse;
				PowerDownDone();
				}
		}
	}




/**
Convert digitiser coordinates to screen coordinates

@param aDigitiserPoint the digitiser coordinates
@param aScreenPoint A TPoint supplied by the caller. 
					On return, set to the converted screen coordinates in pixels.

@return KErrNone if successful
*/
TInt DTemplateDigitiser::DigitiserToScreen(const TPoint& aDigitiserPoint, TPoint& aScreenPoint)
	{
	NKern::LockSystem();
	TInt R11 = iMachineConfig.iCalibration.iR11;
	TInt R12 = iMachineConfig.iCalibration.iR12;
	TInt R21 = iMachineConfig.iCalibration.iR21;
	TInt R22 = iMachineConfig.iCalibration.iR22;
	TInt TX = iMachineConfig.iCalibration.iTx;
	TInt TY = iMachineConfig.iCalibration.iTy;
	NKern::UnlockSystem();
	TInt X = aDigitiserPoint.iX;
	TInt Y = aDigitiserPoint.iY;

	aScreenPoint.iX = (X*R11 + Y*R21 + TX) >> 16;
	aScreenPoint.iY = (X*R12 + Y*R22 + TY) >> 16;

	__KTRACE_OPT(KHARDWARE,Kern::Printf("DtS: Dp.x %d, Dp.y %d, Sp.x %d, Sp.y %d", X,Y,aScreenPoint.iX,aScreenPoint.iY));

	return KErrNone;
	}

/**
Convert screen coordinates back into digitiser coordinates
using the current constants from the superpage

@param aX The screen X coordinate in pixels; On return, set to the digitiser X coordinate.
@param aY The screen Y coordinate in pixels; On return, set to the digitiser Y coordinate.
*/
void DTemplateDigitiser::ScreenToDigitiser(TInt& aX, TInt& aY)
	{
	NKern::LockSystem();
	Int64 R11 = iMachineConfig.iCalibration.iR11;
	Int64 R12 = iMachineConfig.iCalibration.iR12;
	Int64 R21 = iMachineConfig.iCalibration.iR21;
	Int64 R22 = iMachineConfig.iCalibration.iR22;
	Int64 TX = iMachineConfig.iCalibration.iTx;
	Int64 TY = iMachineConfig.iCalibration.iTy;
	NKern::UnlockSystem();
	Int64 X = aX;
	Int64 Y = aY;
	//
	// Xd=(Xs<<16)*R22-(Ys<<16)*R21-(TX*R22)+(TY*R21)
	//	  -------------------------------------------
	//				   (R22*R11)-(R21*R12)
	//
	//
	// Yd=(Xs<<16)*R12-(Ys<<16)*R11-(TX*R12)+(TY*R11)
	//	  -------------------------------------------
	//				   (R21*R12)-(R22*R11)
	//
	// where Xd and Yd are digitiser coordinates
	//		 Xs and Ys are supplied screen coordinates
	//
	X<<=16;
	Y<<=16;

	Int64 d=Int64(R21)*Int64(R12)-Int64(R22)*Int64(R11);

	Int64 r=(X*R12)-(Y*R11)-(TX*R12)+(TY*R11);

	r=r/d;

	aY=(TInt)r;

	r=(X*R22)-(Y*R21)-(TX*R22)+(TY*R21);

	r=r/(-d);

	aX=(TInt)r;
	}

/**
Calculate values for R11, R12, R21, R22, TX and TY

@param aCalibration the screen coordinates of points touched
@return KErrNone if successful
*/
TInt DTemplateDigitiser::SetXYInputCalibration(const TDigitizerCalibration& aCalibration)
	{
	TInt R11,R12,R21,R22,TX,TY;
	//
	// Get coords of expected points
	//
	TDigitizerCalibration cal;
	TInt ret=CalibrationPoints(cal);
	if (ret!=KErrNone)
		return ret;

	TInt Xp1=cal.iTl.iX;
	TInt Yp1=cal.iTl.iY;
	TInt Xp2=cal.iBl.iX;
	TInt Yp2=cal.iBl.iY;
	TInt Xp3=cal.iBr.iX;
	TInt Yp3=cal.iBr.iY;
	//
	// Get coords of points touched in screen coordinates
	//
	TInt X1=aCalibration.iTl.iX;
	TInt Y1=aCalibration.iTl.iY;
	TInt X2=aCalibration.iBl.iX;
	TInt Y2=aCalibration.iBl.iY;
	TInt X3=aCalibration.iBr.iX;
	TInt Y3=aCalibration.iBr.iY;
	//
	// Convert back to raw digitiser coordinates
	//
	ScreenToDigitiser(X1,Y1);
	ScreenToDigitiser(X2,Y2);
	ScreenToDigitiser(X3,Y3);
	//
	// (Y1-Y2)(Xp1-Xp3) - (Y1-Y3)(Xp1-Xp2)
	// ----------------------------------- = R11
	// (Y1-Y2)(X1-X3)	- (Y1-Y3)(X1-X2)
	//
	Int64 r=((Int64(Y1-Y2)*Int64(Xp1-Xp3))-(Int64(Y1-Y3)*Int64(Xp1-Xp2)));
	r<<=16;
	r/=(Int64(Y1-Y2)*Int64(X1-X3)-Int64(Y1-Y3)*Int64(X1-X2));
	R11=(TInt)r;
	//
	// (Y1-Y2)(Yp1-Yp3) - (Y1-Y3)(Yp1-Yp2)
	// ----------------------------------- = R12
	// (Y1-Y2)(X1-X3)	- (Y1-Y3)(X1-X2)
	//
	r=((Int64(Y1-Y2)*Int64(Yp1-Yp3))-(Int64(Y1-Y3)*Int64(Yp1-Yp2)));
	r<<=16;
	r/=(Int64(Y1-Y2)*Int64(X1-X3)-Int64(Y1-Y3)*Int64(X1-X2));
	R12=(TInt)r;
	//
	// (X1-X3)(Xp2-Xp3) - (X2-X3)(Xp1-Xp3)
	// ----------------------------------- = R21
	// (Y2-Y3)(X1-X3)	- (Y1-Y3)(X2-X3)
	//
	r=(((X1-X3)*(Xp2-Xp3))-((X2-X3)*(Xp1-Xp3)));
	r<<=16;
	r/=(Int64(Y2-Y3)*Int64(X1-X3)-Int64(Y1-Y3)*Int64(X2-X3));
	R21=(TInt)r;
	//	
	// (X1-X3)(Yp2-Yp3) - (X2-X3)(Yp1-Yp3)
	// ----------------------------------- = R22
	// (Y2-Y3)(X1-X3)	- (Y1-Y3)(X2-X3)
	//
	r=((Int64(X1-X3)*Int64(Yp2-Yp3))-(Int64(X2-X3)*Int64(Yp1-Yp3)));
	r<<=16;
	r/=(Int64(Y2-Y3)*Int64(X1-X3)-Int64(Y1-Y3)*Int64(X2-X3));
	R22=(TInt)r;
	//
	// TX = Xp1 - X1*R11 - Y1*R21
	//
   TX=(Xp1<<16)-(X1*R11)-(Y1*R21);
	//
	// TY = Yp1 - X1*R12 - Y1*R22
	//
	TY=(Yp1<<16)-(X1*R12)-(Y1*R22);
	
	//
	// Write new values into the superpage
	// 
	NKern::LockSystem();
	iMachineConfig.iCalibration.iR11 = R11;
	iMachineConfig.iCalibration.iR12 = R12;
	iMachineConfig.iCalibration.iR21 = R21;
	iMachineConfig.iCalibration.iR22 = R22;
	iMachineConfig.iCalibration.iTx = TX;
	iMachineConfig.iCalibration.iTy = TY;
	NKern::UnlockSystem();

	return(KErrNone);
	}

/**
Informs the user-side calibration application where to draw 
the cross-hairs on the screen

@param aCalibration On return contains the for points on the screen (in screen coordinates)
					where the cross-hairs should be drawn
@return KErrNone if succcessful
*/
TInt DTemplateDigitiser::CalibrationPoints(TDigitizerCalibration& aCalibration)
	{
	TVideoInfoV01Buf buf;
	TVideoInfoV01& vidinfo=buf();
	TInt r = Kern::HalFunction(EHalGroupDisplay, EDisplayHalCurrentModeInfo, (TAny*)&buf, NULL);
	if (r!=KErrNone)
		return r;
	iScreenSize=vidinfo.iSizeInPixels;

    aCalibration.iBl.iX = aCalibration.iTl.iX = iScreenSize.iWidth/10;
    aCalibration.iTr.iY = aCalibration.iTl.iY = iScreenSize.iHeight/10;
    aCalibration.iBr.iY = aCalibration.iBl.iY = iScreenSize.iHeight-iScreenSize.iHeight/10;
    aCalibration.iTr.iX = aCalibration.iBr.iX = iScreenSize.iWidth-iScreenSize.iWidth/10;
    return r;
	}
/**
Saves the digitiser calibration to the persistent machine configuration area
so that it can be restored after a power-down/up

@return KErrNone if succcessful
*/
TInt DTemplateDigitiser::SaveXYInputCalibration()
	{
	NKern::LockSystem();
	iMachineConfig.iCalibrationSaved = iMachineConfig.iCalibration;
	NKern::UnlockSystem();
	return(KErrNone);
	}

/**
Restores the digitiser calibration from the persistent machine configuration area
following a power-up

@param aType indicates whether to restore factory or saved settings
@return KErrNone if succcessful
*/
TInt DTemplateDigitiser::RestoreXYInputCalibration(TDigitizerCalibrationType aType)
	{
	TInt r=KErrNone;
	NKern::LockSystem();
	switch (aType)
		{
		case EFactory:
			iMachineConfig.iCalibration=iMachineConfig.iCalibrationFactory;
			break;
		case ESaved:
			iMachineConfig.iCalibration=iMachineConfig.iCalibrationSaved;
			break;
		default:
			r=KErrNotSupported;
			break;
		}
	NKern::UnlockSystem();
	return r;
	}

/**
Gets the digitiser configuration information

@param aInfo On return, contains information about the digitiser's dimensions etc.
*/
void DTemplateDigitiser::DigitiserInfo(TDigitiserInfoV01& aInfo)
	{
	__KTRACE_OPT(KEXTENSION,Kern::Printf("DTemplateDigitiser::DigitiserInfo"));
	aInfo.iDigitiserSize.iWidth=KConfigXyWidth;
	aInfo.iDigitiserSize.iHeight=KConfigXyHeight;
	aInfo.iOffsetToDisplay.iX=KConfigXyOffsetX;
	aInfo.iOffsetToDisplay.iY=KConfigXyOffsetY;
	}

/**
Issues a pen move event if the distance from the last point is greater than the threshold

@param aPoint the pen position in screen coordinates
*/
void DTemplateDigitiser::FilterPenMove(const TPoint& aPoint)
	{
	TPoint offset=aPoint;
	offset.iX-=iLastPos.iX;
	offset.iY-=iLastPos.iY;
	if (Abs(offset.iX)>=iCfg.iAccThresholdX || Abs(offset.iY)>=iCfg.iAccThresholdY)
		{
		iLastPos=aPoint;
		IssuePenMoveEvent(aPoint);
		}
	}

/**
Reset the pen move filter
*/
void DTemplateDigitiser::ResetPenMoveFilter()
	{
	}

