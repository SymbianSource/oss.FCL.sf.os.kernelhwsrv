// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// template\template_variant\specific\keyboard.cpp
// Access to Template polled keyboard
// The code here implements a simple polled keyboard driver.
// This is an alternative to the interrupt-driven driver in keyboard_interrupt.cpp.
// This example assumes that we have a non-intelligent keyboard
// consisting of a number of i/o lines arranged in a grid.
// You can use this code as a starting point and modify it to suit
// your hardware.
// 
//

#include <template_assp.h>
#include "platform.h"
#include <kernel/kpower.h>
#include <e32keys.h>



// The TKeyboardState class is used to encapsulate the state of 
// the keyboard. i.e which keys are currently being pressed.
// To determine which keys are being pressed, typically a voltage
// is applied to each row in turn (or column, depending on the hardware) 
// and the output is read resulting in a bitmask for each row.
//
// For example, the keys could be arranged as follows (where a '1' indicates
// that a key is currently being pressed :
// EXAMPLE ONLY
//
//																						Translated
//				Column#	0	1	2	3	4	5	6	7	8	9	A	B	C	D	E	F	KeyCode
//			Row#	
//			6			0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	60	to	6F
//			5			0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	50	to	5F
//			4			0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	40	to	4F
//			3			0	0	0	0	0	0	1	0	0	0	0	0	0	0	0	0	30	to	3F
//	Input->	2			0	0	0	1	0	0	0	0	1	0	0	0	0	0	0	0	20	to	2F
//			1			0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	10	to	1F
//			0			0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	00	to	0F	
//	
//	output->			0	0	0	1	0	0	0	0	1	0	0	0	0	0	0	0	
//
// TO DO: (mandadory)
// Modify TKeyboardState (or provide an alternative) to model the 
// real keyboard state
//
// EXAMPLE ONLY
class TKeyboardState
	{
public:

	enum TDimensions
	{
	KRows = 7,
	KColumns = 16
	};

public:
	TKeyboardState();
	void Clear();
	TBool IsKeyReady();
	TUint32 GetKeyCode();
	TKeyboardState operator&(const TKeyboardState& aState);
	TKeyboardState operator|(const TKeyboardState& aState);
	TKeyboardState operator~();

public:
	TUint32 iKeyBitMask[KRows];
	};

/**
Constructor
*/
TKeyboardState::TKeyboardState()
	{
	Clear();
	}

/**
Clears the array of bitmasks 
*/
void TKeyboardState::Clear()
	{
	for (TInt row=0; row<KRows; row++)
		iKeyBitMask[row] = 0;
	}

/**
Determines whether any keys are being pressed by examining the 
array of bitmasks to determine whether any bits are set

@return	ETrue if one or more keys are being pressed
*/
TBool TKeyboardState::IsKeyReady()
	{
	for (TInt row=0; row<KRows; row++)
		{
		if (iKeyBitMask[row] != 0)
			return ETrue;
		}

	return EFalse;
	}

/**
Scans the array of bitmasks and returns a keycode representing
the first bit that it finds that is on.
E.g. :
if the first bit on the first row is set, then 1 is returned,
if the third bit on the first row is set, then 3 is returned. etc.

Once a bit is found it is cleared to avoid reading it again.

NB Before calling this function, IsKeyReady() should be called 
to determine whether a key code is available.

@return	a 32-bit keycode representing a key that is currently pressed
*/

TUint32 TKeyboardState::GetKeyCode()
	{
	TInt keyNum = 0;
	for (TInt row=0; row<KRows; row++)
		{
		TUint32 bitMask = 1;
		for (TInt col=0; col<KColumns; col++)
			{
			if (iKeyBitMask[row] & bitMask)
				{
				iKeyBitMask[row] &= ~bitMask;
				return keyNum;
				}
			bitMask<<= 1;
			keyNum++;
			}
		}
	return 0;
	}

/**
Perform a bitwise AND between two TKeyboardState objects
by AND-ing together all the 32-bit integers

@return	a new instance of a TKeyboardState object containing the result
*/
TKeyboardState TKeyboardState::operator&(const TKeyboardState& aState)
	{
	TKeyboardState state = *this;

	for (TInt row=0; row<KRows; row++)
		state.iKeyBitMask[row]&= aState.iKeyBitMask[row];;

	return state;
	}

/**
Perform a bitwise OR between two TKeyboardState objects
by OR-ing together all the 32-bit integers

@return	a new instance of a TKeyboardState object containing the result
*/
TKeyboardState TKeyboardState::operator|(const TKeyboardState& aState)
	{
	TKeyboardState state = *this;

	for (TInt row=0; row<KRows; row++)
		state.iKeyBitMask[row]|= aState.iKeyBitMask[row];;

	return state;
	}

/**
Perform a bitwise NOT (one's complement) of a KeyboardState object
by NOT-ing all the 32-bit integers

@return	a new instance of a TKeyboardState object containing the result
*/
TKeyboardState TKeyboardState::operator~()
	{
	TKeyboardState state = *this;

	for (TInt row=0; row<KRows; row++)
		state.iKeyBitMask[row] = ~state.iKeyBitMask[row];

	return state;
	}

//
//
// TO DO: (optional)
//
// Modify this conversion table to suit your keyboard layout
// EXAMPLE ONLY
//

const TUint8 convertCode[] =
	{
//Row 0 (bottom row)
	EStdKeyLeftAlt		,	EStdKeyHash			,	EStdKeyNull			,	EStdKeyLeftCtrl				,
	EStdKeyLeftFunc		,	EStdKeyEscape		,	'1'					,	'2'							,
	'9'					,	'0'					,	EStdKeyMinus		,	EStdKeyEquals				,
	EStdKeyNull			,	EStdKeyBackspace	,	EStdKeyNull			,	EStdKeyNull					,
//Row 1
	EStdKeyNull			,	EStdKeyBackSlash	,	EStdKeyLeftShift	,	EStdKeyNull					,
	EStdKeyNull			,	EStdKeyDelete		,	EStdKeyNull			,	'T'							,
	'Y'					,	'U'					,	'I'					,	 EStdKeyEnter				,
	EStdKeyRightShift	,	EStdKeyDownArrow	,	EStdKeyNull			,	EStdKeyNull					,
//Row 2
	EStdKeyNull			,	EStdKeyTab			,	EStdKeyNull			,	 EStdKeyNull				,
	EStdKeyNull			,	'Q'					,	'W'					,	'E'							,
	'R'					,	'O'					,	'P'					,	EStdKeySquareBracketLeft	,
	EStdKeyNull			,	EStdKeySquareBracketRight,EStdKeyNull		,	EStdKeyNull					,
//Row 3
	EStdKeyNull			,	'Z'					,	EStdKeyNull			,	EStdKeyNull					,
	EStdKeyNull			,	EStdKeyCapsLock		,	EStdKeyNull			,	EStdKeyNull					,
	'K'					,	'L'					,	EStdKeySemiColon	,	EStdKeySingleQuote			,
	EStdKeyNull			,	EStdKeyUpArrow		,	EStdKeyNull			,	EStdKeyNull					,
//Row 4
	EStdKeyNull			,	EStdKeyTab			,	EStdKeyNull			,	EStdKeyNull,
	EStdKeyNull			,	'Q'					,	'W'					,	'E'							,
	'R'					,	'O'					,	'P'					,	EStdKeySquareBracketLeft	,
	EStdKeyNull			,	EStdKeySquareBracketRight,	EStdKeyNull		,	EStdKeyNull					,
//Row 5
	EStdKeyNull			,	'X'					,	EStdKeyNull			,	EStdKeyNull					,
	EStdKeyNull			,	'C'					,	'V'					,	'B'							,
	'N'					,	'M'					,	EStdKeyComma		,	EStdKeyFullStop				,
	EStdKeyNull			,	EStdKeySpace		,	EStdKeyNull			,	EStdKeyNull					,
//Row 6
	EStdKeyNull			,	EStdKeyNull			,	EStdKeyNull			,	EStdKeyNull					,
	EStdKeyNull			,	'3'					,	'4'					,	'5'							,
	'6'					,	'7'					,	'8'					,	EStdKeyMenu					,
	EStdKeyNull			,	EStdKeyRightArrow	,	EStdKeyNull			,	EStdKeyNull					
	};




// EXAMPLE ONLY
const TKeyboard	KConfigKeyboardType = EKeyboard_Full;
const TInt KConfigKeyboardDeviceKeys = 0;
const TInt KConfigKeyboardAppsKeys = 0;


//
// TO DO: (optional)
//
// Set the keyboard scan rate in milliseconds
//

// EXAMPLE ONLY
const TInt KScanRate = 50;	// poll every 1/20 of a second (i.e. every 50 milliseconds)


_LIT(KLitKeyboard,"Keyboard");


//
// TO DO: (optional)
//
// Add any private functions and data you require
//
NONSHARABLE_CLASS(DKeyboardTemplate) : public DPowerHandler
	{
public:
	DKeyboardTemplate();
	TInt Create();
	
	// from DPowerHandler
	void PowerUp();
	void PowerDown(TPowerState);

private:
	static void HandleMessage(TAny* aPtr);
	void HandleMsg(TMessageBase* aMsg);
	
	static TInt HalFunction(TAny* aPtr, TInt aFunction, TAny* a1, TAny* a2);
	TInt HalFunction(TInt aFunction, TAny* a1, TAny* a2);
	
	static void PowerUpDfcFn(TAny* aPtr);
	void PowerUpDfc();
	
	static void PowerDownDfcFn(TAny* aPtr);
	void PowerDownDfc();

	static void TimerCallback(TAny* aDriver);
	static void TimerDfcFn(TAny* aDriver);
	void Poll();

	void KeyboardInfo(TKeyboardInfoV01& aInfo);

	void KeyboardOn();
	void KeyboardOff();
	void KeyboardPowerUp();

private:
	TDfcQue* iDfcQ;
	TMessageQue iMsgQ;	
	TDfc iPowerUpDfc;
	TDfc iPowerDownDfc;	
	TBool iKeyboardOn;
	NTimer iTimer;
	TInt iTimerTicks;
	TDfc iTimerDfc;

	// a bitmask indicating which keys were pressed down on the last timer tick
	TKeyboardState iKeyStateLast;

	// a bitmask indicating the set of keys for which we have sent an EKeyDown event
	TKeyboardState iKeysDown;			
	};

/**
constructor
*/
DKeyboardTemplate::DKeyboardTemplate()
	:	DPowerHandler(KLitKeyboard), 
		iMsgQ(HandleMessage, this, NULL, 1),
		iPowerUpDfc(PowerUpDfcFn, this, 6),
		iPowerDownDfc(PowerDownDfcFn, this, 7),
		iTimer(&DKeyboardTemplate::TimerCallback, (TAny*) this),
		iTimerDfc(TimerDfcFn, this, 1)
	{
	// Convert the scan rate from milliseconds to nanokernel ticks (normally 1/64 of a second)
	iTimerTicks = NKern::TimerTicks(KScanRate);
	}

/**
Second-phase constructor 
Assigns queues for all the DFCs and starts the keyboard-polling timer

Called by factory function at ordinal 0
*/
TInt DKeyboardTemplate::Create()
	{
	iDfcQ=Kern::DfcQue0();

	iKeyboardOn = EFalse;	

	// install the HAL function
	TInt r = Kern::AddHalEntry(EHalGroupKeyboard, DKeyboardTemplate::HalFunction, this);
	if (r != KErrNone)
		return r;

	iTimerDfc.SetDfcQ(iDfcQ);

	iPowerUpDfc.SetDfcQ(iDfcQ);
	iPowerDownDfc.SetDfcQ(iDfcQ);
	iMsgQ.SetDfcQ(iDfcQ);
	iMsgQ.Receive();

	// install the power handler
	Add();

	// Power up the device and start the timer
	KeyboardPowerUp();

	return r;
	}

/**
Calback for the keyboard-polling timer
Called in the context of an ISR

@param	aPtr A pointer to an instance of DKeyboardTemplate
*/
void DKeyboardTemplate::TimerCallback(TAny *aPtr)
	{
	// schedule a DFC
	DKeyboardTemplate& k=*(DKeyboardTemplate*)aPtr;
	k.iTimerDfc.Add();
	}


/**
DFC scheduled by the keyboard-polling timer when it expires

@param	aPtr A pointer to an instance of DKeyboardTemplate
*/
void DKeyboardTemplate::TimerDfcFn(TAny* aPtr)
	{
	((DKeyboardTemplate*)aPtr)->Poll();
	}


/**
Reads scan codes from the keyboard until there are none left
Called from the keyboard-polling timer's DFC
*/
void DKeyboardTemplate::Poll()
	{
	__KTRACE_OPT(KHARDWARE,Kern::Printf("DKeyboardTemplate::EventDfc"));

	
	TKeyboardState keyState;

	//
	// TO DO: (mandatory)
	// Read new key state into the array of bitmasks in keyState
	// This typically involves applying a voltage to each row from 0 to KRows-1, 
	// reading the output state of the i/o lines at every step 
	// - this represents the keys that are pressed on each row -
	// and storing the output of each row as a bitmask into keyState.iKeyBitMask[n], 
	// where n = the row being accessed
	//

	// To enable a simple de-bouncing algorithm, 
	// work out which keys have been pressed down for at least two timer 
	// ticks by AND-ing together the last bitmask with the current bitmask
	TKeyboardState keysStillDown =  keyState & iKeyStateLast;
	

	// Similarly, work out which keys have been "un-pressed" for at least two timer 
	// ticks by AND-ing together the one's complement of the last bitmask with the 
	// one's complement of the current bitmask and 
	// then AND-ing this with the set of keys for which we have sent an EKeyDown 
	// event to give the set of keys for which we need to send an EKeyUp event
	TKeyboardState keysStillUp =  (~keyState & ~iKeyStateLast) & iKeysDown;

	// save the current state for next time
	iKeyStateLast = keyState;

	// update the set of keys for which we have sent an EKeyDown event
	iKeysDown = iKeysDown | keysStillDown;
	iKeysDown = iKeysDown & ~keysStillUp;

	// process all the key-down events
	while (keysStillDown.IsKeyReady())						// while there are keys we haven't processed
		{
		TRawEvent e;
		TUint keyCode = keysStillDown.GetKeyCode();			// Read keycodes from bitmask 

		__KTRACE_OPT(KHARDWARE,Kern::Printf("EKeyDown: #%02x\n",keyCode));

		//
		// TO DO: (mandatory)
		//
		// Convert from hardware scancode to EPOC scancode and send the scancode as an event (key pressed or released)
		// as per below EXAMPLE ONLY:
		//
		__ASSERT_DEBUG(keyCode < (sizeof(convertCode) / sizeof(TUint8)), Kern::Fault("Keyboard", __LINE__));
		TUint8 stdKey = convertCode[keyCode];
		
		e.Set(TRawEvent::EKeyDown, stdKey, 0);
		Kern::AddEvent(e);
		}

	// process all the key-up events
	while (keysStillUp.IsKeyReady())						// while there are keys we haven't processed
		{
		TRawEvent e;
		TUint keyCode = keysStillUp.GetKeyCode();			// Read keycodes from bitmask 

		__KTRACE_OPT(KHARDWARE,Kern::Printf("EKeyUp: #%02x\n",keyCode));

		//
		// TO DO: (mandatory)
		//
		// Convert from hardware scancode to EPOC scancode and send the scancode as an event (key pressed or released)
		// as per below EXAMPLE ONLY:
		//
		__ASSERT_DEBUG(keyCode < (sizeof(convertCode) / sizeof(TUint8)), Kern::Fault("Keyboard", __LINE__));
		TUint8 stdKey = convertCode[keyCode];

		e.Set(TRawEvent::EKeyUp, stdKey, 0);
		Kern::AddEvent(e);
		}

	// start the timer again
	iTimer.OneShot(iTimerTicks);
	}



/**
Notifies the peripheral of system power up.
Called by the power manager during a transition from standby.
Schedules a DFC to handle the power up.
*/
void DKeyboardTemplate::PowerUp()
	{
	iPowerUpDfc.Enque();
	}


/**
static DFC to handle powering up the keyboard

@param	aPtr A pointer to an instance of DKeyboardTemplate
*/
void DKeyboardTemplate::PowerUpDfcFn(TAny* aPtr)
	{
	((DKeyboardTemplate*)aPtr)->PowerUpDfc();
	}


/**
DFC to handle powering up the keyboard
*/
void DKeyboardTemplate::PowerUpDfc()
	{
	__KTRACE_OPT(KPOWER, Kern::Printf("DKeyboardTemplate::PowerUpDfc()"));
	KeyboardOn();

	// Indicate to power handle that powered up is complete
	PowerUpDone();
	}

/**
Powers up the keyboard
May be called as a result of a power transition or from the HAL
*/
void DKeyboardTemplate::KeyboardOn()
	{
	__KTRACE_OPT(KPOWER,Kern::Printf("DKeyboardTemplate::KeyboardOn() iKeyboardOn=%d", iKeyboardOn));

	if (!iKeyboardOn)	// make sure we don't initialize more than once
		KeyboardPowerUp();
	}

/**
Powers up the keyboard
Assumes that the keyboard is currently powered off
*/
void DKeyboardTemplate::KeyboardPowerUp()
	{
	__KTRACE_OPT(KPOWER,Kern::Printf("DKeyboardTemplate::KeyboardPowerUp()"));

	iKeyboardOn = ETrue;

	iKeyStateLast.Clear();
	iKeysDown.Clear();

	// Send key up events for EStdKeyOff (Fn+Esc) event 
	TRawEvent e;
	e.Set(TRawEvent::EKeyUp,EStdKeyEscape,0);
	Kern::AddEvent(e);
	e.Set(TRawEvent::EKeyUp,EStdKeyLeftFunc,0);
	Kern::AddEvent(e);

	// Start the periodic tick for the selected rate.
	// This will call TimerCallback() in the context of an ISR
	iTimer.OneShot(iTimerTicks);
	}


/**
Requests keyboard to power down.
Called by the power manager during a transition to standby or power off
Schedules a DFC to handle the power up.

@param aPowerState the current power state
*/
void DKeyboardTemplate::PowerDown(TPowerState)
	{
	iPowerDownDfc.Enque();
	}

/**
static DFC to handle powering down the keyboard

@param	aPtr A pointer to an instance of DKeyboardTemplate
*/
void DKeyboardTemplate::PowerDownDfcFn(TAny* aPtr)
	{
	((DKeyboardTemplate*)aPtr)->PowerDownDfc();
	}

/**
DFC to handle powering down the keyboard
*/
void DKeyboardTemplate::PowerDownDfc()
	{
	__KTRACE_OPT(KPOWER, Kern::Printf("DKeyboardTemplate::PowerDownDfc()"));
	KeyboardOff();
	PowerDownDone();
	}

/**
Powers down the keyboard
May be called as a result of a power transition or from the HAL
*/
void DKeyboardTemplate::KeyboardOff()
	{
	__KTRACE_OPT(KPOWER,Kern::Printf("DKeyboardTemplate::KeyboardOff() iKeyboardOn=%d", iKeyboardOn));

	// cancel the keyboard-polling timer
	iTimerDfc.Cancel();
	iTimer.Cancel();

	iKeyboardOn = EFalse;
	}


/**
static message handler for processing power up/down messages 
posted internally from HalFunction()

@param	aPtr A pointer to an instance of DKeyboardTemplate
*/
void DKeyboardTemplate::HandleMessage(TAny* aPtr)
	{
	DKeyboardTemplate& h=*(DKeyboardTemplate*)aPtr;
	TMessageBase* pM=h.iMsgQ.iMessage;
	if (pM)
		h.HandleMsg(pM);
	}

/**
Message handler for processing power up/down messages 
posted internally from HalFunction()

param	aMsg A message indicating whether to power the keyboard on or off
*/
void DKeyboardTemplate::HandleMsg(TMessageBase* aMsg)
	{
	if (aMsg->iValue)
		KeyboardOn();
	else
		KeyboardOff();
	aMsg->Complete(KErrNone,ETrue);
	}


/**
Retrieves information about the keyboard
Called from HalFunction()

@param	aInfo a caller-supplied class which on return contains information about the keyboard
*/
void DKeyboardTemplate::KeyboardInfo(TKeyboardInfoV01& aInfo)
	{
	__KTRACE_OPT(KEXTENSION,Kern::Printf("DKeyboardTemplate::KeyboardInfo"));
	aInfo.iKeyboardType=KConfigKeyboardType;
	aInfo.iDeviceKeys=KConfigKeyboardDeviceKeys;
	aInfo.iAppsKeys=KConfigKeyboardAppsKeys;
	}


/**
HAL handler function

@param	aPtr a pointer to an instance of DLcdPowerHandler
@param	aFunction the function number
@param	a1 an arbitrary parameter
@param	a2 an arbitrary parameter
*/
TInt DKeyboardTemplate::HalFunction(TAny* aPtr, TInt aFunction, TAny* a1, TAny* a2)
	{
	DKeyboardTemplate* pH=(DKeyboardTemplate*)aPtr;
	return pH->HalFunction(aFunction,a1,a2);
	}


/**
a HAL entry handling function for HAL group attribute EHalGroupKeyboard

@param	a1 an arbitrary argument
@param	a2 an arbitrary argument
@return	KErrNone if successful
*/
TInt DKeyboardTemplate::HalFunction(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNone;

	__KTRACE_OPT(KEXTENSION,Kern::Printf("DKeyboardTemplate::HalFunction %d", aFunction));
	
	switch(aFunction)
		{
		case EKeyboardHalKeyboardInfo:
			{
			TPckgBuf<TKeyboardInfoV01> kPckg;
			KeyboardInfo(kPckg());
			Kern::InfoCopy(*(TDes8*)a1,kPckg);
			break;
			}

		case EKeyboardHalSetKeyboardState:
			{
			if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EKeyboardHalSetKeyboardState")))
				return KErrPermissionDenied;
			if ((TBool)a1)
				{
				TThreadMessage& m=Kern::Message();
				m.iValue = ETrue;
				m.SendReceive(&iMsgQ);		// send a message and block Client thread until keyboard has been powered up
				}
			else
				{
				TThreadMessage& m=Kern::Message();
				m.iValue = EFalse;
				m.SendReceive(&iMsgQ);		// send a message and block Client thread until keyboard has been powered down
				}
			}
			break;

		case EKeyboardHalKeyboardState:
			kumemput32(a1, &iKeyboardOn, sizeof(TBool));
			break;
		
		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}



DECLARE_STANDARD_EXTENSION()
	{
	__KTRACE_OPT(KEXTENSION,Kern::Printf("Starting keyboard driver"));

	// create keyboard driver
	TInt r=KErrNoMemory;
	DKeyboardTemplate* pK=new DKeyboardTemplate;
	if (pK)
		r=pK->Create();

	__KTRACE_OPT(KEXTENSION,Kern::Printf("Returns %d",r));
	return r;
	}
