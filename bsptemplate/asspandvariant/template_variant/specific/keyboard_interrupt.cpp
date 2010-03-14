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
// template\template_variant\specific\keyboard.cpp
// Access to Template interruptible keyboard
// The code here implements a simple interrupt-driven keyboard driver.
// This is an alternative to the polled driver in keyboard.cpp.
// This example assumes that we have an intelligent keyboard controller
// which:
// scans the keyboard automatically
// generates an interrupt when a key is pressed or released
// You can use this code as a starting point and modify it to suit
// your hardware.
// 
//

#include <template_assp.h>
#include "iolines.h"
#include "platform.h"
#include <kernel/kpower.h>
#include <e32keys.h>
//
//
// TO DO: (optional)
//
// Modify this conversion table to suit your keyboard layout
//
// This example assumes that the following keyboard layout:
//
// <----Row0-----><-----Row1-------><---Row2-----><-----Row3-------><-------Row4-----------><------Row5------><-----Row6-------><---Row7--->
//		LAlt																																Column0
//		'				\				TAB				Z					A						X										Column1
//						LShift																												Column2
//		LCtrl																																Column3
//		Fn																																	Column4
//		Esc				Del				Q				CapLk				S						C				3						Column5
//		1								W									D						V				4						Column6
//		2				T				E									F						B				5						Column7
//		9				Y				R				K					G						N				6						Column8
//		0				U				O				L					H						M				7						Column9
//		-				I				P				;					J						,				8						Column10
//		=				Enter			[				'					/						.				Prog					Column11
//						RShift																												Column12
//		BkSp			DnArrow			]				UpArrow				LeftArrow				Space			RightArrow				Column13
//																																			Column14
//																																			Column15
// EXAMPLE ONLY
const TUint8 convertCode[] =
	{
	EStdKeyNull ,   EStdKeyLeftAlt  ,  EStdKeyNull   ,   EStdKeyNull    ,      EStdKeyNull      ,   EStdKeyNull     ,    EStdKeyNull    ,EStdKeyNull,       // Column0
	EStdKeyNull ,   EStdKeyHash     ,EStdKeyBackSlash,   EStdKeyTab     ,          'Z'          ,       'A'         ,       'X'         ,EStdKeyNull,       // Column1
	EStdKeyNull ,    EStdKeyNull    ,EStdKeyLeftShift,   EStdKeyNull    ,      EStdKeyNull      ,   EStdKeyNull     ,    EStdKeyNull    ,EStdKeyNull,       // Column2
	EStdKeyNull ,  EStdKeyLeftCtrl  ,  EStdKeyNull   ,   EStdKeyNull    ,      EStdKeyNull      ,   EStdKeyNull     ,    EStdKeyNull    ,EStdKeyNull,       // Column3
	EStdKeyNull ,  EStdKeyLeftFunc  ,  EStdKeyNull   ,   EStdKeyNull    ,      EStdKeyNull      ,   EStdKeyNull     ,    EStdKeyNull    ,EStdKeyNull,       // Column4
	EStdKeyNull ,   EStdKeyEscape   , EStdKeyDelete  ,      'Q'         ,      EStdKeyCapsLock  ,       'S'         ,        'C'        ,   '3'     ,       // Column5
	EStdKeyNull ,       '1'         ,  EStdKeyNull   ,      'W'         ,      EStdKeyNull      ,       'D'         ,        'V'        ,   '4'     ,       // Column6
	EStdKeyNull ,       '2'         ,     'T'        ,      'E'         ,      EStdKeyNull      ,       'F'         ,        'B'        ,   '5'     ,       // Column7
	EStdKeyNull ,       '9'         ,     'Y'        ,      'R'         ,          'K'          ,       'G'         ,        'N'        ,   '6'     ,       // Column8
	EStdKeyNull ,       '0'         ,     'U'        ,      'O'         ,          'L'          ,       'H'         ,        'M'        ,   '7'     ,       // Column9
	EStdKeyNull ,    EStdKeyMinus   ,     'I'        ,      'P'         ,    EStdKeySemiColon   ,       'J'         ,    EStdKeyComma   ,   '8'     ,       // Column10
	EStdKeyNull ,    EStdKeyEquals  ,  EStdKeyEnter  ,EStdKeySquareBracketLeft,EStdKeySingleQuote,EStdKeyForwardSlash, EStdKeyFullStop  ,EStdKeyMenu,       // Column11
	EStdKeyNull ,    EStdKeyNull    ,EStdKeyRightShift,   EStdKeyNull   ,      EStdKeyNull      ,   EStdKeyNull     ,    EStdKeyNull    ,EStdKeyNull,       // Column12
	EStdKeyNull ,  EStdKeyBackspace ,EStdKeyDownArrow,EStdKeySquareBracketRight,EStdKeyUpArrow  , EStdKeyLeftArrow  ,    EStdKeySpace   ,EStdKeyRightArrow, // Column13
	EStdKeyNull ,    EStdKeyNull    ,  EStdKeyNull   ,   EStdKeyNull    ,      EStdKeyNull      ,   EStdKeyNull     ,    EStdKeyNull    ,EStdKeyNull,       // Column14
	EStdKeyNull ,    EStdKeyNull    ,  EStdKeyNull   ,   EStdKeyNull    ,      EStdKeyNull      ,   EStdKeyNull     ,    EStdKeyNull    ,EStdKeyNull        // Column15
	};

// EXAMPLE ONLY
const TInt KFlagKeyPressed = 0x80;		// As an example, we'll assume the top bit indicates pressed/released

// EXAMPLE ONLY
const TKeyboard	KConfigKeyboardType = EKeyboard_Full;
const TInt KConfigKeyboardDeviceKeys = 0;
const TInt KConfigKeyboardAppsKeys = 0;
//
//
_LIT(KLitKeyboard,"Keyboard");

class DKeyboardTemplate : public DPowerHandler
	{
public:
	DKeyboardTemplate();
	TInt Create();
public: // from DPowerHandler
	void PowerUp();
	void PowerDown(TPowerState);
	TInt HalFunction(TInt aFunction, TAny* a1, TAny* a2);
	void KeyboardInfo(TKeyboardInfoV01& aInfo);
	void KeyboardOn();
	void KeyboardOff();
	void HandleMsg(TMessageBase* aMsg);
private:
	static void Isr(TAny* aPtr);
	static void EventDfcFn(TAny* aPtr);
	void EventDfc();

	void PowerUpDfc();
	static void PowerUpDfcFn(TAny* aPtr);
	void PowerDownDfc();
	static void PowerDownDfcFn(TAny* aPtr);
private:
	void KeyboardPowerUp();
	TUint GetKeyCode();
	TBool IsKeyReady();
public:
	TDfcQue* iDfcQ;
	TMessageQue iMsgQ;	
	TDfc iPowerUpDfc;
	TDfc iPowerDownDfc;	
private:
	TDfc iEventDfc;
	TBool iKeyboardOn;
	};

LOCAL_C TInt halFunction(TAny* aPtr, TInt aFunction, TAny* a1, TAny* a2)
	{
	DKeyboardTemplate* pH=(DKeyboardTemplate*)aPtr;
	return pH->HalFunction(aFunction,a1,a2);
	}

void rxMsg(TAny* aPtr)
	{
	DKeyboardTemplate& h=*(DKeyboardTemplate*)aPtr;
	TMessageBase* pM=h.iMsgQ.iMessage;
	if (pM)
		h.HandleMsg(pM);
	}

//
//	Keyboard class
//
DKeyboardTemplate::DKeyboardTemplate()
	:	DPowerHandler(KLitKeyboard), 
		iMsgQ(rxMsg,this,NULL,1),
		iPowerUpDfc(PowerUpDfcFn,this,6),
		iPowerDownDfc(PowerDownDfcFn,this,7),
		iEventDfc(EventDfcFn,this,1)
	{
	}

TInt DKeyboardTemplate::Create()
//
// Initialisation. Bind and enable the interrupt.
//
	{
	iDfcQ=Kern::DfcQue0();

	iKeyboardOn = EFalse;	
		// install the HAL function
	TInt r=Kern::AddHalEntry(EHalGroupKeyboard,halFunction,this);
	if (r!=KErrNone)
		return r;

	iEventDfc.SetDfcQ(iDfcQ);
	iPowerUpDfc.SetDfcQ(iDfcQ);
	iPowerDownDfc.SetDfcQ(iDfcQ);
	iMsgQ.SetDfcQ(iDfcQ);
	iMsgQ.Receive();

	// Bind the key event interrupt
	r=Interrupt::Bind(KIntIdKeyboard,Isr,this);
	if (r==KErrNone)
		{
		// install the power handler
		Add();
		KeyboardPowerUp();
		}
	return r;
	}

void DKeyboardTemplate::Isr(TAny* aPtr)
	{
	DKeyboardTemplate& k=*(DKeyboardTemplate*)aPtr;
	Interrupt::Disable(KIntIdKeyboard);

    // Add the timing of key interrupts as entropy data for the RNG
	Interrupt::AddTimingEntropy();

	k.iEventDfc.Add();
	}

void DKeyboardTemplate::EventDfcFn(TAny* aPtr)
	{
	((DKeyboardTemplate*)aPtr)->EventDfc();
	}

void DKeyboardTemplate::EventDfc()
	{
	__KTRACE_OPT(KHARDWARE,Kern::Printf("DKeyboardTemplate::EventDfc"));

	TInt irq=NKern::DisableAllInterrupts();
	while (IsKeyReady())						// while there are keys in the controller's output buffer
		{
		NKern::RestoreInterrupts(irq);
		TRawEvent e;
		TUint keyCode=GetKeyCode();				// Read keycodes from controller
		__KTRACE_OPT(KHARDWARE,Kern::Printf("#%02x",keyCode));

		//
		// TO DO: (mandatory)
		//
		// Convert from hardware scancode to EPOC scancode and send the scancode as an event (key pressed or released)
		// as per below EXAMPLE ONLY:
		//
		TUint bareCode=keyCode&~KFlagKeyPressed;
		TUint8 stdKey=convertCode[bareCode];
		if (keyCode&KFlagKeyPressed)
			e.Set(TRawEvent::EKeyUp,stdKey,0);
		else
			e.Set(TRawEvent::EKeyDown,stdKey,0);
		Kern::AddEvent(e);
		NKern::Sleep(1);						// pause before reading more keycodes
		irq=NKern::DisableAllInterrupts();
		}
	Interrupt::Enable(KIntIdKeyboard);
	NKern::RestoreInterrupts(irq);
	}

TBool DKeyboardTemplate::IsKeyReady()
	{
	//
	// TO DO: (mandatory)
	//
	// Return ETrue if the keyboard controller has a key event waiting to be read
	//

	return EFalse;	// EXAMPLE ONLY
	}

TUint DKeyboardTemplate::GetKeyCode()
	{
	//
	// TO DO: (mandatory)
	//
	// Read and return the next available keycode from the keyboard controller
	//

	return 0;	// EXAMPLE ONLY
	}

void DKeyboardTemplate::PowerUpDfcFn(TAny* aPtr)
	{
	((DKeyboardTemplate*)aPtr)->PowerUpDfc();
	}

void DKeyboardTemplate::PowerDownDfcFn(TAny* aPtr)
	{
	((DKeyboardTemplate*)aPtr)->PowerDownDfc();
	}


void DKeyboardTemplate::KeyboardPowerUp()
	{
	__KTRACE_OPT(KPOWER,Kern::Printf("DKeyboardTemplate::KeyboardPowerUp()"));

	iKeyboardOn = ETrue;

	// Send key up events for EStdKeyOff (Fn+Esc) event 
	TRawEvent e;
	e.Set(TRawEvent::EKeyUp,EStdKeyEscape,0);
	Kern::AddEvent(e);
	e.Set(TRawEvent::EKeyUp,EStdKeyLeftFunc,0);
	Kern::AddEvent(e);
	}

void DKeyboardTemplate::PowerUp()
	{
	iPowerUpDfc.Enque();	// schedules DFC to execute on this driver's thread
	}

void DKeyboardTemplate::PowerUpDfc()
	{
	__KTRACE_OPT(KPOWER, Kern::Printf("DKeyboardTemplate::PowerUpDfc()"));
	KeyboardOn();
	PowerUpDone();		// must be called from a different thread than PowerUp()
	}

void DKeyboardTemplate::KeyboardOn()
	{
	__KTRACE_OPT(KPOWER,Kern::Printf("DKeyboardTemplate::KeyboardOn() iKeyboardOn=%d", iKeyboardOn));

	if (!iKeyboardOn)	// may be powered up from Power Manager or HAL
		{
		KeyboardPowerUp();
		}
	}

void DKeyboardTemplate::PowerDown(TPowerState)
	{
	iPowerDownDfc.Enque();	// schedules DFC to execute on this driver's thread
	}

void DKeyboardTemplate::PowerDownDfc()
	{
	__KTRACE_OPT(KPOWER, Kern::Printf("DKeyboardTemplate::PowerDownDfc()"));
	KeyboardOff();
	PowerDownDone();		// must be called from a different thread than PowerDown()
	}

void DKeyboardTemplate::KeyboardOff()
	{
	__KTRACE_OPT(KPOWER,Kern::Printf("DKeyboardTemplate::KeyboardOff() iKeyboardOn=%d", iKeyboardOn));

	if (iKeyboardOn)	// may have already been powered down by the HAL
		{
		iKeyboardOn = EFalse;
		Interrupt::Disable(KIntIdKeyboard);
		}
	iEventDfc.Cancel();
	}

void DKeyboardTemplate::HandleMsg(TMessageBase* aMsg)
	{
	if (aMsg->iValue)
		KeyboardOn();
	else
		KeyboardOff();
	aMsg->Complete(KErrNone,ETrue);
	}

void DKeyboardTemplate::KeyboardInfo(TKeyboardInfoV01& aInfo)
	{
	__KTRACE_OPT(KEXTENSION,Kern::Printf("DKeyboardTemplate::KeyboardInfo"));

	aInfo.iKeyboardType=KConfigKeyboardType;
	aInfo.iDeviceKeys=KConfigKeyboardDeviceKeys;
	aInfo.iAppsKeys=KConfigKeyboardAppsKeys;
	}

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
