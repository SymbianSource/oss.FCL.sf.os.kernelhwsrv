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
// e32\ewsrv\ws_main.cpp
// 
//

#include "ws_std.h"
#include <e32hal.h>
#include <hal.h>
#include <e32math.h>
#include <domainmanager.h>
#ifdef __WINS__
#include <e32wins.h>
#endif

GLREF_D CKeyTranslator *KeyTranslator;
GLREF_D CKeyRepeat* KeyRepeat;

// password notifier support functions
LOCAL_C void RenderPassword(RConsole *aCon, TInt aPWLeft, const TDesC& aPW);

_LIT(KShellProcessName, "ESHELL");
_LIT(KShellCommandLine, "/p");

//
// class CKeyRepeat
//

CKeyRepeat::CKeyRepeat(TInt aPriority) : CTimer(aPriority)
//
// Constructor. Set default repeat delay and rate
//
    {
    iDelay=EDefaultKeyRepeatDelay;
    iRate=EDefaultKeyRepeatRate;
    }

void CKeyRepeat::ConstructL()
    {

    CTimer::ConstructL();
    CActiveScheduler::Add(this);
    }

void CKeyRepeat::RunL()
//
// Send a repeat keypress to the window
//
    {

    After(iRate);
	CWsWindow::KeyPress(iKeyData);
    }

void CKeyRepeat::Request(TKeyData& aKeyData)
//
// Request a repeat event
//
    {

    iKeyData=aKeyData;
	Cancel();
    After(iDelay);
    }

void CKeyRepeat::SetRepeatTime(TInt aDelay,TInt aRate)
    {

    iDelay=aDelay;
    iRate=aRate;
    }

void CKeyRepeat::RepeatTime(TInt& aDelay,TInt& aRate)
    {

    aDelay=iDelay;
    aRate=iRate;
    }

//
// class CWsSession
//

CWsSession::CWsSession()
	{
	iTestFast = (UserSvr::DebugMask(2)&0x00000002) ? 1 : 0;
	}

CWsSession::~CWsSession()
//
// Destructor
//
	{

	delete iWindow;
	}

//
// class CWsServer
//

void CWsServer::New()
//
// Create a new CWsServer.
//
	{

	CWsServer *pS=new CWsServer(EPriority);
	__ASSERT_ALWAYS(pS!=NULL,Fault(ECreateServer));
	pS->SetPinClientDescriptors(EFalse); // don't pin because client interface can't cope with errors if pin fails under real or simulated OOM
	TInt r=pS->Start(KE32WindowServer);
	__ASSERT_ALWAYS(r==KErrNone,Fault(EStartServer));
	RProcess::Rendezvous(KErrNone);

	}

CWsServer::CWsServer(TInt aPriority)
//
// Constructor.
//
	: CServer2(aPriority)
	{
	}

CSession2* CWsServer::NewSessionL(const TVersion& aVersion,const RMessage2&) const
//
// Create a new client for this server.
//
	{

	TVersion v(KW32MajorVersionNumber,KW32MinorVersionNumber,KE32BuildVersionNumber);
	TBool r=User::QueryVersionSupported(v,aVersion);
	if (!r)
		User::Leave(KErrNotSupported);
	return new(ELeave) CWsSession;
	}

void CWsSession::ServiceL(const RMessage2& aMessage)
//									    
// Handle messages for this session.
//
	{

	iCurMsg = aMessage;
	CWsWindow::WaitOnService();
	CWsWindow* pW=iWindow;
	TInt r=EPrematureOperation;
	TBool delayCompletion=EFalse;
	switch (aMessage.Function())
		{
	case EConsoleCreate:
		{
		if (pW)
			{
			delete pW;
			iWindow=NULL;
			}
		pW=new CWsWindow;
		if (!pW)
			{ 
			r=EWindowOutOfMemory;
			break;
			}
		iWindow=pW;
		pW->iAllowResize=ETrue;
		pW->iIsVisible=ETrue;
		pW->iOnTop=EFalse;
		pW->SetCursorHeight(50);
		pW->iKQueue.SetOffset(_FOFF(SWsKey,iLink));
		break;
		}
	case EConsoleSet:
		{
		if (!pW)
			{
			pW=new(ELeave) CWsWindow;
			iWindow=pW;
			pW->iAllowResize=ETrue;
			pW->iIsVisible=ETrue;
			pW->iOnTop=EFalse;
			pW->SetCursorHeight(50);
			pW->iKQueue.SetOffset(_FOFF(SWsKey,iLink));
			}
		pW->iAllowSlide=ETrue;
		TFileName name;
		iCurMsg.ReadL(0, name);
		pW->iTitle=name;
		TPckgBuf<TSize> size;
		iCurMsg.ReadL(1, size);
		TRAP(r,pW->CreateL(size()));
		if (r != KErrNone)
			{
			delete pW;
			iWindow=NULL;			
			}
		break;
		}
	case EConsoleClearScreen:
		{
		if (pW)
			{
			pW->Clear();
			r=KErrNone;
			}
		break;
		}
	case EConsoleClearToEndOfLine:
		{
		if (pW)
			{
			pW->ClearToEndOfLine();
			r=KErrNone;
			}
		break;
		}
	case EConsoleSetTitle:
		{
		if (pW)
			{
			TFileName name;
			iCurMsg.ReadL(0, name);
			pW->SetTitle(name);
			r=KErrNone;
			}
		break;
		}
	case EConsoleSetSize:
		{
		if (pW)
			{
			TPckgBuf<TSize> size;
			iCurMsg.ReadL(0, size);
//			pW->SetSize(size());
//			r=KErrNone;
			r=KErrNotSupported;
			}
		break;
		}
	case EConsoleSetWindowPosAbs:
		{
		if (pW)
			{
			TPckgBuf<TPoint> point;
			iCurMsg.ReadL(0, point);
			pW->SetWindowPosAbs(point());
			r=KErrNone;
			}
		break;
		}
	case EConsoleSetCursorHeight:
		{
		if (pW)
			{
			pW->SetCursorHeight(aMessage.Int0());
			r=KErrNone;
			}
		break;
		}
	case EConsoleSetCursorPosAbs:
		{
		if (pW)
			{
			TPckgBuf<TPoint> point;
			iCurMsg.ReadL(0, point);
			pW->SetCursorPosAbs(point());
			r=KErrNone;
			}
		break;
		}
	case EConsoleSetCursorPosRel:
		{
		if (pW)
			{
			TPckgBuf<TPoint> point;
			iCurMsg.ReadL(0, point);
			pW->SetCursorPosRel(point());
			r=KErrNone;
			}
		break;
		}
	case EConsoleCursorPos:
		{
		if (pW)
			{
			TPckgBuf<TPoint> point;
			point()=pW->CursorPosition();
			aMessage.WriteL(0,point);
			r=KErrNone;
			}
		break;
		}
	case EConsoleSize:
		{
		if (pW)
			{
			TPckgBuf<TSize> size;
			size()=pW->Size();
			aMessage.WriteL(0,size);
			r=KErrNone;
			}
		break;
		}
	case EConsoleScreenSize:
		{
		if (pW)
			{
			TPckgBuf<TSize> size;
			size()=CWsWindow::ScreenSize;
			aMessage.WriteL(0,size);
			r=KErrNone;
			}
		break;
		}
	case EConsoleControl:
		{
		if (pW)
			{
			TBool indicator=ETrue;
			TInt offset=0;
			TBuf<0x100> b;
			do
				{
				iCurMsg.ReadL(0,b,offset);
				for (const TText* pB=b.Ptr();pB<b.Ptr()+b.Length();pB++)
					{
					switch(*pB)
						{
					case '+':

						indicator=ETrue;
						break;
					case '-':
						indicator=EFalse;
						break;
					case 'S':
						pW->ControlScrollBars(indicator);
						break;
					case 'W':
						pW->ControlWrapLock(indicator);
						break;
                    case 'P':
                        pW->ControlPointerEvents(indicator);
                        break;
					case 'L':
						pW->ControlScrollLock(indicator);
						break;
					case 'V':
						pW->ControlVisibility(indicator);
						break;
					case 'C':
						pW->ControlCursorRequired(indicator);
						break;
					case 'M':
						pW->ControlMaximised(indicator);
						break;
					case 'N':
						pW->ControlNewLineMode(indicator);
						break;
                    case 'O':
						pW->ControlOnTop(indicator);
						break;
					case 'I':
                        pW->ControlInformAllMouse(indicator);
                        break;
                    case 'R':
                        pW->ControlRawEventMode(indicator);
                        break;
					case 'A':
						pW->ControlAllowResize(indicator);
						}
					}
				offset+=b.Length();
				}
			while (b.Length()==b.MaxLength());
			r=KErrNone;
			}
		break;
		}
	case EConsoleWrite:
		{
		if (pW)
			{
			switch(iTestFast)
				{
			case 0:
				{
				TInt offset=0;
				TBuf<0x100> b;
				do
					{
					iCurMsg.ReadL(0,b,offset);
					pW->Write(b);
					offset+=b.Length();
					}
				while (b.Length()==b.MaxLength());
				pW->WriteDone();
				}
				r=KErrNone;
				break;

			case 1:
				{
				pW->Write(_L("Output suppressed because TESTFAST mode is set..."));
				pW->WriteDone();
				++iTestFast;
				}
				r=KErrNone;
				break;

			default:
				r=KErrNone;
				break;
				}
			}
		break;
		}
	case EConsoleRead:
		{
		if (pW)
			{
			if (pW->EnqueReadRequest(aMessage))
				{
				delayCompletion=ETrue;
				r=KErrNone;
				}
			else
				r=EDoubleReadRequest;
			}
		break;
		}
	case EConsoleReadCancel:
		{
		if (pW)
			{
			pW->DequeReadRequest();
			r=KErrNone;
			}
		break;
		}
	case EConsoleDestroy:
		{
		if (pW)
			{
			delete pW;
			iWindow=NULL;
			r=KErrNone;
			}
		break;
		}
	case EConsoleSetMode:
		{
		r=CWsWindow::SetMode((TVideoMode)aMessage.Int0());		
		break;
		}
	case EConsoleSetPaletteEntry:
		{
		CWsWindow::ScreenDriver->SetPaletteEntry((TColorIndex)aMessage.Int0(),(TUint8)aMessage.Int1(),(TUint8)aMessage.Int2(),(TUint8)aMessage.Int3());
		pW->Redraw();
		r=KErrNone;
		break;
		}
	case EConsoleGetPaletteEntry:
		{
		TUint8 r,g,b;
		TPckgBuf<TUint8> red,green,blue;
		CWsWindow::ScreenDriver->GetPaletteEntry((TColorIndex)aMessage.Int0(),r,g,b);
		red()=r; green()=g; blue()=b;
		aMessage.WriteL(1,red);
		aMessage.WriteL(2,green);
		aMessage.WriteL(3,blue);
		}
		r=KErrNone;
		break;
	case EConsoleSetTextColors:
		{
		if(pW)
			{
			pW->iFgColor=(TColorIndex)aMessage.Int0();
			pW->iBgColor=(TColorIndex)aMessage.Int1();
			}		
		r=KErrNone;
		break;
		}
	case EConsoleSetUIColors:
		{
		CWsWindow::WindowBgColor=(TColorIndex)aMessage.Int0();
		CWsWindow::BorderColor=(TColorIndex)aMessage.Int1();
		CWsWindow::ScreenColor=(TColorIndex)aMessage.Int2();
		CWsWindow::ChangeUIColors();
		r=KErrNone;
		break;
		}
	case EConsoleSetTextAttribute:
		{
		if(pW)
			pW->SetTextAttribute((TTextAttribute)aMessage.Int0());		
		r=KErrNone;
		break;
		}
	default:
		r=KErrNotSupported;
		}
	if (!delayCompletion)
		aMessage.Complete(r);
	CWsWindow::SignalService();
	}

void CWsSession::ServiceError(const RMessage2& aMessage,TInt aError)
	{
	if (!aMessage.IsNull())
		{
		if (aError>0)
			{
			aMessage.Panic(_L("WServ panic"),aError);
			}
		else
			{
			aMessage.Complete(aError);
			}
		}
	}

CWsServer::~CWsServer()
//
// Destructor
//
	{
	}

//
// class CEvent
//

void CEvent::New()
//
// Create the CEvent active object.
//
	{

	CEvent *pE=new CEvent(EPriority);
	__ASSERT_ALWAYS(pE!=NULL,Fault(ECreateEvent));
	pE->CaptureKeys=new CCaptureKeys();
	__ASSERT_ALWAYS(pE->CaptureKeys!=NULL,Fault(ECreateEvent));

	pE->CaptureKeys->Construct();

	CActiveScheduler::Add(pE);
	UserSvr::CaptureEventHook();
	pE->Request();
	}

CEvent::~CEvent()
//
// Destroy the CEvent active object
//
	{

	Cancel();
	}

#pragma warning( disable : 4705 )	// statement has no effect
CEvent::CEvent(TInt aPriority)
//
// Constructor
//
	: CActive(aPriority)
	{
	}
#pragma warning( default : 4705 )

void CEvent::Request()
//
// Issue a request for the next event.
//
	{

	UserSvr::RequestEvent(iEvent,iStatus);
	SetActive();
	}

void CEvent::DoCancel()
//
// Cancel a pending event.
//
	{

	UserSvr::RequestEventCancel();
	}

void CEvent::RunL()
//
// Event has completed.
//
	{

    if (CWsWindow::RawEventMode())
        {
        KeyRepeat->Cancel();
        CWsWindow::QueueRawEvent(iEvent.Event());
        Request();
        return;
        }
	switch(iEvent.Event().Type())
		{
	case TRawEvent::ERedraw:
		CWsWindow::Redraw();
		break;
	case TRawEvent::EButton1Down:
        if(!CWsWindow::MouseIsCaptured)
            {
      		CWsWindow::MouseMove(iEvent.Event().Pos());
  	    	CWsWindow::MouseLeftButton();
            }
        else
            CWsWindow::InformTopMouse(iEvent.Event().Pos());
		break;
	case TRawEvent::EButton1Up:
		if(!CWsWindow::MouseIsCaptured)
			{
			CWsWindow::MouseMove(iEvent.Event().Pos());
			CWsWindow::MouseLeftButtonUp();
			}
		break;
	case TRawEvent::EButton2Down:
		break;
	case TRawEvent::EButton2Up:
		break;
	case TRawEvent::EButton3Down:
		break;
	case TRawEvent::EButton3Up:
		break;
	case TRawEvent::EPointerMove:
		CWsWindow::MouseMove(iEvent.Event().Pos());
		break;
	case TRawEvent::EInactive:
        KeyRepeat->Cancel();
        break;
    case TRawEvent::EActive:
        break;
    case TRawEvent::EUpdateModifiers:
        KeyTranslator->UpdateModifiers(iEvent.Event().Modifiers());
        break;
	case TRawEvent::EKeyDown:
		{
		TKeyData keyData;
		if (KeyTranslator->TranslateKey(iEvent.Event().ScanCode(), EFalse,*CaptureKeys,keyData))
			CWsWindow::KeyPress(keyData);
		if (keyData.iModifiers&EModifierAutorepeatable)
			KeyRepeat->Request(keyData);
		break;
		}
	case TRawEvent::EKeyUp:
		{
		TKeyData keyData;
		KeyRepeat->Cancel();
		if (KeyTranslator->TranslateKey(iEvent.Event().ScanCode(), ETrue,*CaptureKeys,keyData))
			CWsWindow::KeyPress(keyData);
		break;
		}
	case TRawEvent::ESwitchOn:
	case TRawEvent::ECaseOpen:
		HAL::Set(HAL::EDisplayState, 1);
			{
			RDmDomainManager mgr; 
			TInt r = mgr.Connect();
			if (r != KErrNone)
				User::Panic(_L("EWSRV SwitchOn0"), r);
			TRequestStatus status;
			mgr.RequestDomainTransition(KDmIdUiApps, EPwActive, status);
			User::WaitForRequest(status);
			if (status.Int() != KErrNone)
				User::Panic(_L("EWSRV SwitchOn1"), status.Int());
			mgr.Close();
			}
		break;
	case TRawEvent::EPointerSwitchOn:
#if defined(_DEBUG)
    	User::Beep(440,250000);
#endif
		break;
	case TRawEvent::ESwitchOff:
			{
			RDmDomainManager mgr; 
			TInt r = mgr.Connect();
			if (r != KErrNone)
				User::Panic(_L("EWSRV SwitchOff0"), r);
			TRequestStatus status;
			mgr.RequestSystemTransition(EPwStandby, status);
			User::WaitForRequest(status);
			if (status.Int() != KErrNone)
				User::Panic(_L("EWSRV SwitchOff1"), status.Int());
			mgr.Close();
			}
		break;
	case TRawEvent::ECaseClose:
			{
			RDmDomainManager mgr; 
			TInt r = mgr.Connect();
			if (r != KErrNone)
				User::Panic(_L("EWSRV CaseClosed"), r);
			TRequestStatus status;
			mgr.RequestDomainTransition(KDmIdUiApps, EPwStandby, status);
			User::WaitForRequest(status);
			if (status.Int() != KErrNone)
				User::Panic(_L("EWSRV CaseClosed1"), status.Int());
			mgr.Close();
			}
		HAL::Set(HAL::EDisplayState, 0);
		break;
	case TRawEvent::ENone:
		break;
	default:
		break;
		}                                       
	Request();
	}

LOCAL_C void RenderPassword(RConsole *aCon, TInt aPWLeft, const TDesC& aPW)
// pre:		aCon points to a console being used to read a password.
//			aPWLeft is the column number from which the left brace should be drawn.
//			aPasswd is a valid password.
// post:	the password is rendered onto the console and followed by a '#' and
//			padding spaces.  Everything is enclosed in a pair of square brackets.
	{
	aCon->SetCursorPosAbs(TPoint(aPWLeft, 3));
	aCon->Write(_L("["));
	aCon->Write(aPW);
	aCon->Write(_L("#"));

	TInt i;
	for (i = 0; i < KMaxMediaPassword - aPW.Length(); i++)
		{
		aCon->Write(_L(" "));
		}

	aCon->Write(_L("]"));
	}


void CNotifierSession::RunPasswordWindowL(const RMessage2 &aMsg)
//
// Eight unicode chars are mapped to (up to) sixteen bytes.  Remainder of array is
// zeroed.  Message is completed in CNotifierSession::ServiceL().
// 
	{
	// local copies of dialog data, 5 * (8 + 32 * 2) bytes
	TBuf<0x20> line1, line2, unlockBtn, storeBtn, cancelBtn;
	line1.Copy(_L("Password notifier"));
	line2.Copy(_L("Enter password"));
	unlockBtn.Copy(_L("Unlock"));
	storeBtn.Copy(_L("Store"));
	cancelBtn.Copy(_L("Cancel"));

	TPckgBuf<TMediaPswdReplyNotifyInfoV1> reply;

	// Format the console window.

	const TInt KPasswordBarLen(1 + KMaxMediaPassword + 1 + 1);
	const TInt KButtonBarLen(
				1 + unlockBtn.Length() + 1
		+ 1 +	1 + cancelBtn.Length() + 1
		+ 1 +	1 + storeBtn.Length() + 1);

	// Work out width of window.
	// (Buttons are enclosed by angle brackets and separted by a space.)
	// (Password is followed by '#' and delimited by square brackets.)
	// If KButtonBarLen is at least as long as any other line then it will write
	// to the bottom right corner and cause the console to scroll.  To counter
	// this, an extra padding character is added if necessary.

	TInt width;
	width = Max(line1.Length(), line2.Length());
	width = Max(width, KPasswordBarLen);
	width = KButtonBarLen >= width ? KButtonBarLen + 1: width;

	// Create the window and render its contents.

	RConsole con;
	con.Create();
	con.Control(_L("-Visible"));
	TInt r = con.Init(_L(""), TSize(width, 2 + 1 + 1 + 1 + 1));
	if (KErrNone != r)
		{
		PanicClient(aMsg, ENotifierPanicPasswordWindow);
		User::Leave(KErrGeneral);
		}
	con.Control(_L("+Max -Cursor -AllowResize +OnTop"));

	con.SetCursorPosAbs(TPoint(0, 0));
	con.Write(line1);
	con.SetCursorPosAbs(TPoint(0, 1));
	con.Write(line2);
	const TInt KPasswordLeft((width - KPasswordBarLen) / 2);
	con.SetCursorPosAbs(TPoint(KPasswordLeft, 3));
	con.Write(_L("[#                ]"));
	con.SetCursorPosAbs(TPoint((width - KButtonBarLen) / 2, 5));
	con.Write(_L("<"));
	con.Write(unlockBtn);
	con.Write(_L("> <"));
	con.Write(storeBtn);
	con.Write(_L("> <"));
	con.Write(cancelBtn);
	con.Write(_L(">"));

	// Allow the user to edit the password until they either press enter or escape.

	TBool done(EFalse);
	TBuf<KMaxMediaPassword> pw;
	pw.Zero();
	TMediaPswdNotifyExitMode em(EMPEMUnlock);	// avoid VC warning C4701 (used w/o init).

	const TInt sendInfoLen = User::LeaveIfError(aMsg.GetDesLength(1));

	if (sendInfoLen == sizeof(TMediaPswdSendNotifyInfoV1Debug))
		{
		// debug mode - wait for specified period and close notifier

		TPckgBuf<TMediaPswdSendNotifyInfoV1Debug> sendDbg;
		aMsg.ReadL(1, sendDbg);
		if (sendDbg().iSleepPeriod >= 0)
			User::After(sendDbg().iSleepPeriod);
		else
			{
			TTime now;
			now.HomeTime();
			TInt64 seed = now.Int64();
			User::After(Math::Rand(seed) % -(sendDbg().iSleepPeriod));
			}

		reply().iEM = sendDbg().iEM;
		Mem::Copy(reply().iPW, sendDbg().iPW, KMaxMediaPassword);
		}
	else
		{
		RenderPassword(&con, KPasswordLeft, pw);
		do
			{
			TConsoleKey key;

			con.Read(key);
			TInt keyCode = key.Code();

			switch (keyCode)
				{
				case EKeyEscape:
					em = EMPEMCancel;
					done = ETrue;
					break;

				case EKeyEnter:
					em = EMPEMUnlock;
					done = ETrue;
					break;
				
				case EKeySpace:
					em = EMPEMUnlockAndStore;
					done = ETrue;
					break;
				
				case EKeyBackspace:
					if (pw.Length() > 0)
						{
						pw.SetLength(pw.Length() - 1);
						RenderPassword(&con, KPasswordLeft, pw);
						}
					break;

				default:							// interpret other keys as pw contents
					TChar ch(keyCode);
					// unicode encoding, so number of password characters is half byte length
					if (ch.IsPrint() && pw.Length() < KMaxMediaPassword / 2)
						{
						pw.Append(ch);
						RenderPassword(&con, KPasswordLeft, pw);
						}
					break;
				}
			} while (! done);

		// Fill the TMediaPswdReplyNotifyInfoV1 structure.

		if (em == EMPEMUnlock || em == EMPEMUnlockAndStore)
			{
			const TInt byteLen = pw.Length() * 2;

			// zero entire array; and then copy in valid section of TMediaPassword,
			// not converting Unicode to ASCII.
			TPtr8 pt8(reply().iPW, KMaxMediaPassword);	// length = 0
			pt8.FillZ(KMaxMediaPassword);				// length = KMaxMediaPassword
			pt8.Zero();									// length = 0
														// length = byteLen
			pt8.Copy(reinterpret_cast<const TUint8 *>(pw.Ptr()), byteLen);
			}
		
		// Set exit mode to tell user how dialog handled.

		reply().iEM = em;
		}	// else (sendInfoLen == sizeof(TMediaPswdSendNotifyInfoV1Debug))
	
	con.Destroy();

	aMsg.WriteL(2, reply);
	}

//
// class MNotifierBase2
//

void MNotifierBase2::SetManager(MNotifierManager* aManager)
	{
	iManager=aManager;
	}

//
// class CNotifierServer
//

_LIT(__NOTIFIER_SERVER,"TextNotifierSrvr");

CNotifierServer* CNotifierServer::NewL()
	{
	CNotifierServer* server=new(ELeave) CNotifierServer(200);
	CleanupStack::PushL(server);
	server->ConstructL();
	server->StartL(__NOTIFIER_NAME);
	CleanupStack::Pop(); // server
	return server;
	}

CNotifierServer::~CNotifierServer()
	{
	SetIsExiting();
	delete iManager;
	}

void CNotifierServer::SetIsExiting()
	{
	iExiting=ETrue;
	}

TBool CNotifierServer::IsExiting() const
	{
	return iExiting;
	}

CNotifierServer::CNotifierServer(TInt aPriority)
	: CServer2(aPriority)
	{}

void CNotifierServer::ConstructL()
	{
	iManager=CNotifierManager::NewL();
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);
	iManager->RegisterL(fs);
	CleanupStack::PopAndDestroy(); // fs.Close()
	}

CSession2* CNotifierServer::NewSessionL(const TVersion &aVersion,const RMessage2&) const
	{
	TVersion v(1,0,0); // !! liaise with E32
	if (!User::QueryVersionSupported(v,aVersion))
		User::Leave(KErrNotSupported);
	return new(ELeave) CNotifierSession(*this);
	}

//
// class CNotifierSession
//

CNotifierSession::CNotifierSession(const CNotifierServer& aServer)
	{
	iServer=&aServer;
	iClientId=(TInt)this;
	}

CNotifierSession::~CNotifierSession()
	{
	const CNotifierServer* server=static_cast<const CNotifierServer*>(Server());
	if (!server->IsExiting())
		{
		server->Manager()->HandleClientExit(iClientId);
		}
	}

void CNotifierSession::ServiceL(const RMessage2 &aMessage)
	{
	TBool completeMessage=ETrue;
	switch (aMessage.Function())
		{
	case ENotifierNotify:
		DisplayAlertL(aMessage);
		break;
	case ENotifierNotifyCancel:
		// do nothing - this server doesn't support cancelling RNotifier::Notify - the client will just have to wait
		break;
	case ENotifierInfoPrint:
		DisplayInfoMsgL(aMessage);
		break;
	case EStartNotifier:
		DoStartNotifierL(aMessage);
		break;
	case ECancelNotifier:
		iServer->Manager()->NotifierCancel(TUid::Uid(aMessage.Int0()));
		break;
	case EUpdateNotifierAndGetResponse:
	case EUpdateNotifier:
		DoUpdateNotifierL(aMessage);
		break;
	case EStartNotifierAndGetResponse:
		{
		if (aMessage.Int0()==KMediaPasswordNotifyUid)
			{
			RunPasswordWindowL(aMessage);
			}
		else
			{
			TBool cleanupComplete=ETrue;
			StartNotifierAndGetResponseL(aMessage,cleanupComplete);
			 // if the plug-in starts successfully, it has
			 // responsibility for completing the message (either
			 // synchronously or asynchronously)
			completeMessage=EFalse;
			}
		}
		break;
	default:
		aMessage.Complete(KErrNotSupported);
		break;
		}
	if (completeMessage && !aMessage.IsNull())
		{
		aMessage.Complete(KErrNone);
		}
	}

void CNotifierSession::DisplayAlertL(const RMessage2& aMessage)
	{
	const TInt lengthOfCombinedBuffer=User::LeaveIfError(aMessage.GetDesLength(1));
	const TInt lengthOfLine1=(STATIC_CAST(TUint,aMessage.Int2())>>16);
	const TInt lengthOfLine2=(aMessage.Int2()&KMaxTUint16);
	const TInt lengthOfBut1=(STATIC_CAST(TUint,aMessage.Int3())>>16);
	const TInt lengthOfBut2=(aMessage.Int3()&KMaxTUint16);
	if (lengthOfCombinedBuffer!=lengthOfLine1+lengthOfLine2+lengthOfBut1+lengthOfBut2)
		{
		PanicClient(aMessage,ENotifierPanicInconsistentDescriptorLengths);
		return;
		}
	HBufC* const combinedBuffer=HBufC::NewLC(lengthOfCombinedBuffer);
	{TPtr combinedBuffer_asWritable(combinedBuffer->Des());
	aMessage.ReadL(1,combinedBuffer_asWritable);}
	const TPtrC line1(combinedBuffer->Left(lengthOfLine1));
	const TPtrC line2(combinedBuffer->Mid(lengthOfLine1,lengthOfLine2));
	const TPtrC but1(combinedBuffer->Mid(lengthOfLine1+lengthOfLine2,lengthOfBut1));
	const TPtrC but2(combinedBuffer->Mid(lengthOfLine1+lengthOfLine2+lengthOfBut1,lengthOfBut2));
	TInt buttons, len, offset;
	if (lengthOfBut2==0)
		{
		buttons=1;
		len=lengthOfBut1+2;
		}
	else
		{
		buttons=2;
		len=lengthOfBut1+lengthOfBut2+5;
		}
	if (lengthOfLine1>len)
		len=lengthOfLine1;
	if (lengthOfLine2>len)
		len=lengthOfLine2;
	RConsole con;
	con.Create();
	TSize scsz;
	con.ScreenSize(scsz);
	con.Control(_L("-Visible"));
	TInt ww=Min(len,scsz.iWidth-4);
	TInt wh=3;
	if ((lengthOfBut1+lengthOfBut2+5)>ww)
		wh++;
	if (lengthOfLine1>ww)
		wh++;
	if (lengthOfLine2>ww)
		wh++;
	con.Init(_L(""),TSize(ww,wh));
	con.Control(_L("+Max -Cursor -Allowresize +Ontop +Wrap"));
	con.Write(line1);
	con.SetCursorPosAbs(TPoint(0,1));
	con.Write(line2);
	if (buttons==2)
		offset=(len-lengthOfBut1-lengthOfBut2-5)/2;
	else
		offset=(len-lengthOfBut1-2)/2;
	con.SetCursorPosAbs(TPoint(offset,2));
	con.Write(_L("<"));
	con.Write(but1);
	con.Write(_L(">"));
	if(buttons==2)
		{
		con.Write(_L(" <"));
		con.Write(but2);
		con.Write(_L(">"));
		}
	
	TConsoleKey key;
	TInt keycode;
	do
		{
		con.Read(key);
		keycode=key.Code();
		}
	while((keycode!=EKeyEscape&&keycode!=EKeyEnter&&buttons==2)||(keycode!=EKeyEnter&&buttons==1));
	if(keycode==EKeyEscape)
		keycode=0;
	else
		keycode=1;
	con.Destroy();
	aMessage.WriteL(0,TPckgC<TInt>(keycode));
	CleanupStack::PopAndDestroy(combinedBuffer);
	}

TInt CNotifierSession::InfoPrintThread(TAny* aMessage)
	{
	TBuf<0x50> des; // 0x50 max size of message
	RConsole con;
	des=*(TBuf<0x50> *)aMessage;
	TInt l=des.Length();
	NotifierSemaphore.Signal();
	con.Create();
	con.Control(_L("-Visible"));
	TSize size;
	con.ScreenSize(size);
	TInt ww=Min(size.iWidth-6,l);
	TInt wh=(l+ww-1)/ww;
	if (wh==0)
		wh=1;
	con.Init(_L(""),TSize(ww,wh));
	con.Control(_L("+Maximise"));
	con.SetWindowPosAbs(TPoint(size.iWidth-ww-4,1));
	con.Control(_L("+Wrap +Ontop"));
	con.Write(des);

	User::After(1300000);
	con.Destroy();
	return KErrNone;
	}

void CNotifierSession::DisplayInfoMsgL(const RMessage2& aMessage)
	{
	TInt r;
	TBuf<0x50> des; // 0x50 max size of message lines
	aMessage.ReadL(0,des);
	RThread thread;
	do
		{
		r=thread.Create(_L("Info Window"),InfoPrintThread,KDefaultStackSize,&User::Allocator(),(TAny*)&des);
		if(r==KErrAlreadyExists)
			User::After(200000);
		} while(r==KErrAlreadyExists);
		User::LeaveIfError(r);
	thread.Resume();
	NotifierSemaphore.Wait();
	thread.Close();
	}

void CNotifierSession::DoStartNotifierL(const RMessage2& aMessage)
	{
	const TUid targetUid={aMessage.Int0()};
	HBufC8* const inputBuffer=HBufC8::NewLC(User::LeaveIfError(aMessage.GetDesLength(1)));
	{TPtr8 input(inputBuffer->Des());
	aMessage.ReadL(1,input);}
	TPtrC8 output(0,0);
	iServer->Manager()->NotifierStartL(targetUid,*inputBuffer,&output,iClientId);
	if(aMessage.Int2())
		aMessage.WriteL(2,output);
	CleanupStack::PopAndDestroy(inputBuffer);
	}

void CNotifierSession::DoUpdateNotifierL(const RMessage2& aMessage)
	{
	const TUid targetUid={aMessage.Int0()};
	HBufC8* const inputBuffer=HBufC8::NewLC(User::LeaveIfError(aMessage.GetDesLength(1)));
	{TPtr8 input(inputBuffer->Des());
	aMessage.ReadL(1, input);}
	HBufC8* const outputBuffer=HBufC8::NewLC(User::LeaveIfError(aMessage.GetDesMaxLength(2)));
	{TPtr8 output(outputBuffer->Des());
	iServer->Manager()->NotifierUpdateL(targetUid,*inputBuffer,&output,iClientId);}
	aMessage.WriteL(2,*outputBuffer);
	CleanupStack::PopAndDestroy(2,inputBuffer);
	}

void CNotifierSession::StartNotifierAndGetResponseL(const RMessage2& aMessage,TBool& aCleanupComplete)
	{
	const TUid targetUid={aMessage.Int0()};
	HBufC8* const inputBuffer=HBufC8::NewLC(User::LeaveIfError(aMessage.GetDesLength(1)));
	{TPtr8 input(inputBuffer->Des());
	aMessage.ReadL(1,input);}
	iServer->Manager()->NotifierStartAndGetResponseL(targetUid,*inputBuffer,2,aMessage,iClientId,aCleanupComplete);
	CleanupStack::PopAndDestroy(inputBuffer);
	}

void CNotifierSession::PanicClient(const RMessage2& aMessage,TNotifierPanic aCode)
	{
	aMessage.Panic(__NOTIFIER_SERVER,aCode);
	}

//
// class CNotifierManager
//

const TInt KNullClientId=0;

CNotifierManager* CNotifierManager::NewL()
	{
	CNotifierManager* self=new(ELeave) CNotifierManager;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CNotifierManager::~CNotifierManager()
	{
	if (iObservedList)
		{
		const TInt count=iObservedList->Count();
		for (TInt ii=0;ii<count;ii++)
			{
			(*iObservedList)[ii]->Release();
			}
		delete iObservedList;
		}
	if (iLibraries)
		{
		const TInt count=iLibraries->Count();
		for (TInt ii=0;ii<count;ii++)
			{
			(*iLibraries)[ii].Close();
			}
		delete iLibraries;
		}
	delete iChannelMonitor;
	delete iActivityMonitor;
	delete iQueue;
	}


void CNotifierManager::RegisterL(RFs& aFs)
	{
#ifdef SUPPORT_OLD_PLUGIN_PATH
	TBool old;
	for(old=0; old<2; old++)
	{
#endif
	TFindFile* findFile=new(ELeave) TFindFile(aFs);
	CleanupStack::PushL(findFile);
	TParse* fileNameParser=new(ELeave) TParse;
	CleanupStack::PushL(fileNameParser);
	CDir* directory=NULL;
	TInt error;
#ifdef SUPPORT_OLD_PLUGIN_PATH
	_LIT(KNotifierPlugInOldSearchPath,"\\system\\tnotifiers\\");
	if(old)
		error=findFile->FindWildByDir(KNotifierPlugInExt, KNotifierPlugInOldSearchPath, directory);
	else
#endif
	error=findFile->FindWildByDir(KNotifierPlugInExt, KNotifierPlugInSearchPath, directory);
	for (; error!=KErrNotFound; error=findFile->FindWild(directory))
		{
		CleanupStack::PushL(directory);
		User::LeaveIfError(error);
		const TInt numberOfEntries=directory->Count();
		for (TInt i=0; i<numberOfEntries; ++i)
			{
			const TEntry& entry=(*directory)[i];
			fileNameParser->SetNoWild(entry.iName, &findFile->File(), NULL); // findFile->File() returns a reference rather than an object, therefore taking the address of it is fine

			if (entry.iType[0].iUid==0x10000079)
				{
				 // It's a DLL
				if( (entry.iType[1]==KUidTextNotifierPlugInV2))
					{
					// Its a notifier...
					TPtrC path(fileNameParser->DriveAndPath());
					TPtrC name(fileNameParser->NameAndExt());
					DoAddPlugInL(path,name,entry.iType);
					}
				}
			}
		CleanupStack::PopAndDestroy(); // directory
		directory=NULL;
		}
	delete directory;
	CleanupStack::PopAndDestroy(2); // fileNameParser and findFile
#ifdef SUPPORT_OLD_PLUGIN_PATH
	}
#endif
	}

LOCAL_C void DeleteTemp(TAny* aPtr)
	{
	CArrayPtr<MNotifierBase2>* array=REINTERPRET_CAST(CArrayPtr<MNotifierBase2>*,aPtr);
	const TInt count=array->Count();
	for (TInt ii=0;ii<count;ii++)
		{
		(*array)[ii]->Release();
		}
	delete array;
	}



void CNotifierManager::DoAddPlugInL(const TDesC& aPath,const TDesC& aFileName,const TUidType& aUidType)
	{
	RLibrary lib;
	User::LeaveIfError(lib.Load(aFileName,aPath,aUidType));
	CleanupClosePushL(lib);
	iLibraries->AppendL(lib);
	CleanupStack::Pop(); // lib
	TLibraryFunction libEntry=lib.Lookup(1);
	CArrayPtr<MNotifierBase2>* array=REINTERPRET_CAST(CArrayPtr<MNotifierBase2>*,(libEntry)());
	User::LeaveIfNull(array);
	CleanupStack::PushL(TCleanupItem(DeleteTemp,array));
	while (array->Count()>0)
		{
		MNotifierBase2* notif=(*array)[0];
			{
			iObservedList->AppendL(notif);
			array->Delete(0);
			notif->SetManager(this);
			}
		MNotifierBase2::TNotifierInfo info=notif->RegisterL();
		if (!iChannelMonitor->AlreadyHasChannel(info.iChannel))
			iChannelMonitor->AddNewChannelL(info.iChannel);
		}
	CleanupStack::PopAndDestroy(); // array
	}

CNotifierManager::CNotifierManager()
	{}

void CNotifierManager::ConstructL()
	{
	iObservedList=new(ELeave) CArrayPtrSeg<MNotifierBase2>(6);
	iLibraries=new(ELeave) CArrayFixFlat<RLibrary>(2);
	iChannelMonitor=CChannelMonitor::NewL();
	iActivityMonitor=CActivityMonitor::NewL();
	iQueue=CNotifierQueue::NewL();
	}

struct SActivityCleanup
	{
	CActivityMonitor* iMonitor;
	TUid iNotifier;
	TInt iClientId;
	};

LOCAL_C void CleanupActivityMonitor(TAny* aPtr)
	{
	SActivityCleanup& cleanup=*REINTERPRET_CAST(SActivityCleanup*,aPtr);
	cleanup.iMonitor->Remove(cleanup.iNotifier,cleanup.iClientId);
	}

void CNotifierManager::NotifierStartL(TUid aNotifierUid,const TDesC8& aBuffer,TPtrC8* aResponse,TInt aClientId)
	{
	TInt result=KErrNotFound;
	const TInt count=iObservedList->Count();
	for (TInt ii=0;ii<count;ii++)
		{
		MNotifierBase2* notif=(*iObservedList)[ii];
		MNotifierBase2::TNotifierInfo info=notif->Info();
		if (info.iUid==aNotifierUid)
			{
			if (iActivityMonitor->IsNotifierActive(aNotifierUid,info.iChannel))
				{
				result=KErrAlreadyExists;
				}
			else if (info.iPriority>iChannelMonitor->ActivityLevel(info.iChannel))
				{
				TUid notifier;
				MNotifierBase2::TNotifierPriority priority;
				const TBool channelWasActive=iActivityMonitor->IsChannelActive(info.iChannel,notifier,priority);
				iActivityMonitor->AddL(info,aClientId);
				SActivityCleanup cleanup;
				cleanup.iMonitor=iActivityMonitor;
				cleanup.iNotifier=aNotifierUid;
				cleanup.iClientId=aClientId;
				CleanupStack::PushL(TCleanupItem(CleanupActivityMonitor,&cleanup));
				TPtrC8 response(notif->StartL(aBuffer));
				if(aResponse)
					aResponse->Set(response);
				CleanupStack::Pop(); // cleanup;
				if (channelWasActive)
					{
					for (TInt jj=0;jj<count;jj++)
						{
						MNotifierBase2* notifForUpdate=(*iObservedList)[ii];
						MNotifierBase2::TNotifierInfo infoForUpdate=notifForUpdate->Info();
						if (infoForUpdate.iUid==notifier && infoForUpdate.iChannel==info.iChannel)
							{
							TRAP_IGNORE(notifForUpdate->UpdateL(KNotifierPaused));
							}
						}
					}
				iChannelMonitor->UpdateChannel(info.iChannel,info.iPriority);
				if (result!=ENotExtRequestQueued)
					{
					result=ENotExtRequestCompleted;
					}
				}
			else
				{
				if (iQueue->IsAlreadyQueued(info.iUid,info.iChannel))
					{
					result=KErrAlreadyExists;
					}
				else
					{
					CQueueItem* queueCopy=CQueueItem::NewL(info,aBuffer,aClientId);
					CleanupStack::PushL(queueCopy);
					iQueue->QueueItemL(queueCopy);
					CleanupStack::Pop(); // queueCopy
					result=ENotExtRequestQueued;
					}
				}
			}
		}
	User::LeaveIfError(result);
	}

TInt CNotifierManager::NotifierUpdateL(TUid aNotifierUid,const TDesC8& aBuffer,TDes8* aResponse,TInt aClientId)
	{
	TInt result=KErrNotFound;
	const TInt count=iObservedList->Count();
	for (TInt ii=0;ii<count;ii++)
		{
		MNotifierBase2* notif=(*iObservedList)[ii];
		MNotifierBase2::TNotifierInfo info=notif->Info();
		if (info.iUid==aNotifierUid)
			{
			if (iActivityMonitor->IsNotifierActive(aNotifierUid,info.iChannel))
				{
				if (!iActivityMonitor->IsClientPresent(aNotifierUid,info.iChannel,aClientId))
					{
					iActivityMonitor->AddL(info,aClientId);
					}
				if (aResponse==NULL)
					{
					notif->UpdateL(aBuffer);
					}
				else
					{
					aResponse->Copy(notif->UpdateL(aBuffer));
					}
				}
			else
				{
				; // not all channels have been started yet so update the queue
				}
			result=KErrNone;
			}
		}
	return result;
	}

void CNotifierManager::NotifierStartAndGetResponseL(TUid aNotifierUid,const TDesC8& aBuffer,TInt aReplySlot,
														  const RMessage2& aMessage,TInt aClientId,TBool& aCleanupComplete)
	{
	NotifierStartAndGetResponseL(aNotifierUid,TUid::Null(),aBuffer,aReplySlot,aMessage,aClientId,aCleanupComplete);
	}

void CNotifierManager::NotifierStartAndGetResponseL(TUid aNotifierUid,TUid aChannelUid,const TDesC8& aBuffer,TInt aReplySlot,
														  const RMessage2& aMessage,TInt aClientId,TBool& aCleanupComplete)
	{
	TInt result=KErrNotFound;
	const TInt count=iObservedList->Count();
	for (TInt ii=0;ii<count;ii++)
		{
		MNotifierBase2* notif=(*iObservedList)[ii];
		MNotifierBase2::TNotifierInfo info=notif->Info();
		if (info.iUid==aNotifierUid && (aChannelUid==TUid::Null() || info.iChannel==aChannelUid))
			{
			if (iActivityMonitor->IsNotifierActive(aNotifierUid,info.iChannel))
				{
				notif->StartL(aBuffer,aReplySlot,aMessage); // asynch notifier can decide whether to support multiple clients
				result=KErrNone;
				}
			else if (info.iPriority>iChannelMonitor->ActivityLevel(info.iChannel))
				{
				TUid notifier;
				MNotifierBase2::TNotifierPriority priority;
				const TBool channelWasActive=iActivityMonitor->IsChannelActive(info.iChannel,notifier,priority);
				iActivityMonitor->AddL(info,aClientId);
				SActivityCleanup activityCleanup;
				activityCleanup.iMonitor=iActivityMonitor;
				activityCleanup.iNotifier=aNotifierUid;
				activityCleanup.iClientId=aClientId;
				CleanupStack::PushL(TCleanupItem(CleanupActivityMonitor,&activityCleanup));
				aCleanupComplete=EFalse;
				// IMPORTANT, aMessage needs to be a full RMessage object until suport for V1 notifiers is removed
				// I.e. until CNotifierBaseAdaptor is removed
				notif->StartL(aBuffer,aReplySlot,aMessage);
				CleanupStack::Pop(&activityCleanup);
				if (channelWasActive)
					{
					for (TInt jj=0;jj<count;jj++)
						{
						MNotifierBase2* notifForUpdate=(*iObservedList)[ii];
						MNotifierBase2::TNotifierInfo infoForUpdate=notifForUpdate->Info();
						if (infoForUpdate.iUid==notifier && infoForUpdate.iChannel==info.iChannel)
							{
							TRAP_IGNORE(notifForUpdate->UpdateL(KNotifierPaused));
							}
						}
					}
				iChannelMonitor->UpdateChannel(info.iChannel,info.iPriority);
				result=KErrNone;
				}
			else
				{
				if (iQueue->IsAlreadyQueued(info.iUid,info.iChannel))
					{
					result=KErrAlreadyExists;
					}
				else
					{
					CQueueItem* queueCopy=CQueueItem::NewL(info,aBuffer,aReplySlot,aMessage,aClientId);
					CleanupStack::PushL(queueCopy);
					iQueue->QueueItemL(queueCopy);
					CleanupStack::Pop(queueCopy);
					result=ENotExtRequestQueued;
					}
				}
			}
		}
	User::LeaveIfError(result);
	}

TInt CNotifierManager::NotifierCancel(TUid aNotifierUid)
	{
	TInt result=KErrNotFound;
	const TInt count=iObservedList->Count();
	for (TInt ii=0;ii<count;ii++)
		{
		MNotifierBase2* notif=(*iObservedList)[ii];
		MNotifierBase2::TNotifierInfo info=notif->Info();
		if (info.iUid==aNotifierUid)
			{
			notif->Cancel();
			iActivityMonitor->RemoveNotifier(aNotifierUid,info.iChannel);
			MNotifierBase2::TNotifierPriority priority=MNotifierBase2::ENotifierPriorityLowest;
			TUid notifier;
			//check channel activity and get highest priority on channnel
			if (iActivityMonitor->IsChannelActive(info.iChannel,notifier,priority))
				{

				//check if priority of a queued item on the same channel is
				//greater 
				MNotifierBase2::TNotifierPriority queuePriority=
				(MNotifierBase2::TNotifierPriority)iQueue->GetHighestQueuePriority(info.iChannel);
				if (queuePriority>priority)
					{
					iChannelMonitor->UpdateChannel(info.iChannel,MNotifierBase2::ENotifierPriorityLowest);
					CQueueItem* next=iQueue->FetchItem(info.iChannel);
					if (next)
						{
						TUid notif=next->iInfo.iUid;
						TRAPD(err,StartFromQueueL(next));
						if (err!=KErrNone)
							{
							NotifierCancel(notif);
							}
						}
					 }
					else
					{
					for (TInt jj=0;jj<count;jj++)
						{
						MNotifierBase2* notifForUpdate=(*iObservedList)[ii];
						MNotifierBase2::TNotifierInfo infoForUpdate=notifForUpdate->Info();
						if (infoForUpdate.iUid==notifier && infoForUpdate.iChannel==info.iChannel)
							{
							TRAP_IGNORE(notifForUpdate->UpdateL(KNotifierPaused));
							}
						}
					iChannelMonitor->UpdateChannel(info.iChannel,priority);
					}
				}
			else
				{
				iChannelMonitor->UpdateChannel(info.iChannel,MNotifierBase2::ENotifierPriorityLowest);
				CQueueItem* next=iQueue->FetchItem(info.iChannel);
				if (next)
					{
					TUid notif=next->iInfo.iUid;
					TRAPD(err,StartFromQueueL(next));
					if (err!=KErrNone)
						{
						NotifierCancel(notif);
						}
					}
				}
			result=KErrNone;
			}
		}
	return result;
	}

struct SCleanupMessage
	{
	TBool* iDoCleanup;
	RMessage2* iMessage;
	};

LOCAL_C void CleanupStartAndGetResponse(TAny* aPtr)
	{
	SCleanupMessage& cleanup=*REINTERPRET_CAST(SCleanupMessage*,aPtr);
	if (cleanup.iDoCleanup)
		{
		cleanup.iMessage->Complete(KErrNoMemory);
		}
	}

void CNotifierManager::StartFromQueueL(CQueueItem* aItem)
	{
	CleanupStack::PushL(aItem);
	TPtr8 buffer=aItem->iBuffer->Des();
	if (aItem->iAsynchronous)
		{
		SCleanupMessage cleanup;
		TBool doCleanup=ETrue;
		cleanup.iDoCleanup=&doCleanup;
		cleanup.iMessage=&aItem->iMessage;
		CleanupStack::PushL(TCleanupItem(CleanupStartAndGetResponse,&cleanup));
		// IMPORTANT, aItem->iMessage needs to be a full RMessage object until suport for V1 notifiers is removed
		// I.e. until CNotifierBaseAdaptor is removed
		NotifierStartAndGetResponseL(aItem->iInfo.iUid,aItem->iInfo.iChannel,buffer,aItem->iReplySlot,aItem->iMessage,aItem->iClientId,doCleanup);
		CleanupStack::Pop(&cleanup);
		}
	else
		{
		NotifierStartL(aItem->iInfo.iUid,buffer,NULL,aItem->iClientId);
		}
	CleanupStack::PopAndDestroy(); // aItem
	CQueueItem* update=iQueue->FetchItem(aItem->iInfo.iChannel);
	while (update)
		{
		CleanupStack::PushL(update);
		NotifierUpdateL(update->iInfo.iUid,*update->iBuffer,NULL,update->iClientId);
		CleanupStack::PopAndDestroy(); // update
		update=iQueue->FetchItem(aItem->iInfo.iChannel);
		}
	}

void CNotifierManager::HandleClientExit(TInt aClientId)
	{
	TUid notifier=KNullUid;
	while (iActivityMonitor->NotifierForClient(notifier,aClientId))
		{
		const TInt count=iObservedList->Count();
		for (TInt ii=0;ii<count;ii++)
			{
			MNotifierBase2* notif=(*iObservedList)[ii];
			if (notif->Info().iUid==notifier)
				{
				NotifierCancel(notifier);
				}
			}
		iActivityMonitor->Remove(notifier,aClientId);
		}
	iActivityMonitor->RemoveClient(aClientId);
	iQueue->RemoveClient(aClientId);
	}

void CNotifierManager::StartNotifierL(TUid aNotifierUid,const TDesC8& aBuffer,TDes8& aResponse)
	{
	TPtrC8 response(0,0);
	NotifierStartL(aNotifierUid,aBuffer, &response,KNullClientId);
	aResponse.Copy(response);
	}

void CNotifierManager::CancelNotifier(TUid aNotifierUid)
	{
	NotifierCancel(aNotifierUid);
	}

void CNotifierManager::UpdateNotifierL(TUid aNotifierUid,const TDesC8& aBuffer,TDes8& aResponse)
	{
	NotifierUpdateL(aNotifierUid,aBuffer,&aResponse,KNullClientId);
	}

//
// class CChannelMonitor
//

CChannelMonitor* CChannelMonitor::NewL()
	{
	CChannelMonitor* self=new(ELeave) CChannelMonitor;
	return self;
	}

TBool CChannelMonitor::AlreadyHasChannel(TUid aChannel)const
	{
	const TInt count=iMonitor.Count();
	for (TInt ii=0;ii<count;ii++)
		{
		if (iMonitor[ii].iChannel==aChannel)
			return ETrue;
		}
	return EFalse;
	}

TInt CChannelMonitor::ActivityLevel(TUid aChannel) const
	{
	const TInt count=iMonitor.Count();
	for (TInt ii=0;ii<count;ii++)
		{
		TChannelActivity activity=iMonitor[ii];
		if (activity.iChannel==aChannel)
			return activity.iHighestPriorityRunning;
		}
	return 0;
	}

void CChannelMonitor::UpdateChannel(TUid aChannel,TInt aLevel)
	{
	const TInt count=iMonitor.Count();
	for (TInt ii=0;ii<count;ii++)
		{
		TChannelActivity& activity=iMonitor[ii];
		if (activity.iChannel==aChannel)
			{
			activity.iHighestPriorityRunning=aLevel;
			break;
			}
		}
	}

CChannelMonitor::CChannelMonitor()
	:iMonitor(3)
	{}

//
// class CNotifierActivity
//

CNotifierActivity* CNotifierActivity::NewLC(const MNotifierBase2::TNotifierInfo& aInfo,TInt aClientId)
	{ // static
	CNotifierActivity* self=new(ELeave) CNotifierActivity(aInfo);
	CleanupStack::PushL(self);
	self->ConstructL(aClientId);
	return self;
	}

CNotifierActivity::~CNotifierActivity()
	{
	iClientArray.Reset();
	}

TInt CNotifierActivity::Find(TInt aClientId) const
	{
	TInt index=KErrNotFound;
	const TInt count=iClientArray.Count();
	for (TInt ii=0;ii<count;ii++)
		{
		TInt clientId=iClientArray[ii];
		if (clientId==aClientId)
			{
			index=ii;
			break;
			}
		}
	return index;
	}

CNotifierActivity::CNotifierActivity(const MNotifierBase2::TNotifierInfo& aInfo)
	: iInfo(aInfo), iClientArray(1)
	{}

void CNotifierActivity::ConstructL(TInt aClientId)
	{
	iClientArray.AppendL(aClientId);
	}

//
// class CActivityMonitor
//

CActivityMonitor* CActivityMonitor::NewL()
	{ // static
	CActivityMonitor* self=new(ELeave) CActivityMonitor();
	return self;
	}

CActivityMonitor::~CActivityMonitor()
	{
	iMonitor.ResetAndDestroy();
	}

void CActivityMonitor::AddL(const MNotifierBase2::TNotifierInfo& aInfo,TInt aClientId)
	{
	const TInt index=Find(aInfo.iUid,aInfo.iChannel);
	if (index==KErrNotFound)
		{
		CNotifierActivity* activity=CNotifierActivity::NewLC(aInfo,aClientId);
		iMonitor.AppendL(activity);
		CleanupStack::Pop(); // activity
		}
	else
		{
		iMonitor[index]->iClientArray.AppendL(aClientId);
		}
	}

void CActivityMonitor::Remove(TUid aNotifierUid,TInt aClientId)
	{
	const TInt index=Find(aNotifierUid);
	if (index!=KErrNotFound)
		{
		CNotifierActivity* activity=iMonitor[index];
		const TInt clientIndex=activity->Find(aClientId);
		if (clientIndex!=KErrNotFound)
			{
			if (activity->iClientArray.Count()==1)
				{
				delete activity;
				iMonitor.Delete(index);
				}
			else
				{
				activity->iClientArray.Delete(index);
				}
			}
		}
	}

void CActivityMonitor::RemoveNotifier(TUid aNotifierUid,TUid aChannel)
	{
	const TInt index=Find(aNotifierUid,aChannel);
	if (index!=KErrNotFound)
		{
		delete iMonitor[index];
		iMonitor.Delete(index);
		}
	}

void CActivityMonitor::RemoveClient(TInt aClientId)
	{
	TInt ii=0;
	while (ii<iMonitor.Count())
		{
		CNotifierActivity* ptr=iMonitor[ii];
		TInt index=ptr->Find(aClientId);
		if (index!=KErrNotFound)
			{
			ptr->iClientArray.Delete(index);
			}
		if (ptr->iClientArray.Count()==0)
			{
			iMonitor.Delete(ii);
			}
		else
			{
			++ii;
			}
		}
	}

TBool CActivityMonitor::IsNotifierActive(TUid aNotifierUid,TUid aChannel) const
	{
	const TInt index=Find(aNotifierUid,aChannel);
	return (index!=KErrNotFound);
	}

TBool CActivityMonitor::IsClientPresent(TUid aNotifierUid,TUid aChannel,TInt aClientId) const
	{
	TBool found=EFalse;
	const TInt index=Find(aNotifierUid,aChannel);
	if (index!=KErrNotFound)
		{
		found=(iMonitor[index]->Find(aClientId)!=KErrNotFound);
		}
	return found;
	}

TBool CActivityMonitor::IsChannelActive(TUid aChannel,TUid& aNotifier,MNotifierBase2::TNotifierPriority& aHighestPriority) const
	{
	TBool ret=EFalse;
	const TInt count=iMonitor.Count();
	for (TInt ii=0;ii<count;ii++)
		{
		MNotifierBase2::TNotifierInfo info=iMonitor[ii]->iInfo;
		if (info.iChannel==aChannel)
			{
			ret=ETrue;
			if ((MNotifierBase2::TNotifierPriority)info.iPriority>aHighestPriority)
				{
				aNotifier=info.iUid;
				aHighestPriority=(MNotifierBase2::TNotifierPriority)info.iPriority;
				}
			}
		}
	return ret;
	}

TBool CActivityMonitor::NotifierForClient(TUid& aNotifierUid,TInt aClientId) const
	{
	TBool isOnlyClient=EFalse;
	aNotifierUid=KNullUid;
	const TInt count=iMonitor.Count();
	for (TInt ii=0;ii<count;ii++)
		{
		CNotifierActivity* ptr=iMonitor[ii];
		if (ptr->Find(aClientId)!=KErrNotFound)
			{
			aNotifierUid=ptr->iInfo.iUid;
			isOnlyClient=ptr->iClientArray.Count()==1;
			break;
			}
		}
	return isOnlyClient;
	}

CActivityMonitor::CActivityMonitor()
	: iMonitor(1)
	{}

TInt CActivityMonitor::Find(TUid aNotifierUid) const
	{
	TInt index=KErrNotFound;
	const TInt count=iMonitor.Count();
	for (TInt ii=0;ii<count;ii++)
		{
		if (iMonitor[ii]->iInfo.iUid==aNotifierUid)
			{
			index=ii;
			break;
			}
		}
	return index;
	}

TInt CActivityMonitor::Find(TUid aNotifierUid,TUid aChannel) const
	{
	TInt index=KErrNotFound;
	const TInt count=iMonitor.Count();
	for (TInt ii=0;ii<count;ii++)
		{
		CNotifierActivity* ptr=iMonitor[ii];
		if (ptr->iInfo.iUid==aNotifierUid && ptr->iInfo.iChannel==aChannel)
			{
			index=ii;
			break;
			}
		}
	return index;
	}

//
// class CQueueItem
//

CQueueItem* CQueueItem::NewL(const MNotifierBase2::TNotifierInfo& aInfo,const TDesC8& aBuffer,
									TInt aReplySlot,const RMessage2& aMessage,TInt aClientId) //Asynchronous
	{
	CQueueItem* self=new(ELeave) CQueueItem(aInfo);
	CleanupStack::PushL(self);
	self->ConstructL(aBuffer,aMessage,aClientId,aReplySlot);
	CleanupStack::Pop(); // self
	return self;
	}

CQueueItem* CQueueItem::NewL(const MNotifierBase2::TNotifierInfo& aInfo,const TDesC8& aBuffer,TInt aClientId) //synchronous
	{
	CQueueItem* self=new(ELeave) CQueueItem(aInfo);
	CleanupStack::PushL(self);
	self->ConstructL(aBuffer,aClientId);
	CleanupStack::Pop(); // self
	return self;
	}

CQueueItem::~CQueueItem()
	{
	delete iBuffer;
	}

CQueueItem::CQueueItem(const MNotifierBase2::TNotifierInfo& aInfo)
	: iInfo(aInfo)
	{}

void CQueueItem::ConstructL(const TDesC8& aBuffer,TInt aClientId)
	{
	iBuffer=aBuffer.AllocL();
	iClientId=aClientId;
	iAsynchronous=EFalse;
	}

void CQueueItem::ConstructL(const TDesC8& aBuffer,const RMessage2& aMessage,TInt aClientId,TInt aReplySlot)
	{
	iBuffer=aBuffer.AllocL();
	iAsynchronous=ETrue;
	iMessage=aMessage;
	iClientId=aClientId;
	iReplySlot=aReplySlot;
	}

//
// class CNotifierQueue
//

CNotifierQueue* CNotifierQueue::NewL()
	{
	CNotifierQueue* self=new(ELeave) CNotifierQueue;
	return self;
	}

CQueueItem* CNotifierQueue::FetchItem(TUid aChannel)
	{
	CQueueItem* result=NULL;
	const TInt count=iQueue.Count();
	TInt priority=MNotifierBase2::ENotifierPriorityLowest-1;
	TInt index=KErrNotFound;
	for (TInt ii=0;ii<count;ii++)
		{
		CQueueItem* item=iQueue[ii];
		if (item->iInfo.iChannel==aChannel && item->iInfo.iPriority>priority)
			{
			index=ii;
			priority=item->iInfo.iPriority;
			result=item;
			}
		}
	if (index!=KErrNotFound)
		{
		iQueue.Delete(index);
		}
	return result;
	}

TBool CNotifierQueue::IsAlreadyQueued(TUid aNotifier,TUid aChannel) const
	{
	TBool ret=EFalse;
	const TInt count=iQueue.Count();
	for (TInt ii=0;ii<count;ii++)
		{
		CQueueItem* item=iQueue[ii];
		if (item->iInfo.iUid==aNotifier && item->iInfo.iChannel==aChannel)
			{
			ret=ETrue;
			break;
			}
		}
	return ret;
	}

void CNotifierQueue::RemoveClient(TInt aClientId)
	{
	const TInt count=iQueue.Count();
	for (TInt ii=count-1;ii>=0;ii--)
		{
		CQueueItem* item=iQueue[ii];
		TInt clientId=item->iClientId;
		if (clientId==aClientId)
			{
			iQueue.Delete(ii);
			}
		}
	}


TInt CNotifierQueue::GetHighestQueuePriority(TUid aChannel)
	{
	const TInt count=iQueue.Count();
	TInt priority=MNotifierBase2::ENotifierPriorityLowest-1;

	for (TInt ii=0;ii<count;ii++)
		{
		CQueueItem* item=iQueue[ii];
		if (item->iInfo.iChannel==aChannel && item->iInfo.iPriority>priority)
			{
			priority=item->iInfo.iPriority;
			}
		}

	return priority;
	}


void CWsActiveScheduler::New()
//
// Create and install the active scheduler.
//
	{

	CWsActiveScheduler *pA=new CWsActiveScheduler;
	__ASSERT_ALWAYS(pA!=NULL,Fault(ECreateScheduler));
	CActiveScheduler::Install(pA);
	}

void CWsActiveScheduler::Error(TInt) const
//
// Called if any Run() method leaves.
//
	{
	}


TInt NotifierServerThread(TAny*)
	{
	CTrapCleanup* CleanUpStack=CTrapCleanup::New();
	CWsActiveScheduler::New();
	TRAP_IGNORE(CNotifierServer::NewL());
	CNotifierSession::NotifierSemaphore.Signal();
	CWsActiveScheduler::Start();
	delete CleanUpStack;
	return(0);
	}


_LIT(KLitKeyDataDllNameBase, "EKDATA");
_LIT(TwoDigExt,".%02d");

GLDEF_C TInt E32Main()
	{
	UserSvr::WsRegisterThread();
	UserSvr::WsRegisterSwitchOnScreenHandling(ETrue);
	User::SetProcessCritical(User::ESystemPermanent);
	User::SetCritical(User::ESystemPermanent);

	CWsActiveScheduler::New();
	CWsServer::New();
	CWsWindow::New();
	CEvent::New();
    
	KeyTranslator=CKeyTranslator::New();
	if (!KeyTranslator)
		Fault(ENoKeyboardTranslator);

//  Change keyboard mapping according to information in the HAL
//	This code is the same as WSERV
	TInt keyboardIndex;
	if (HAL::Get(HALData::EKeyboardIndex,keyboardIndex)==KErrNone)
		{
		TBuf<16> keyDataDllName(KLitKeyDataDllNameBase);
		keyDataDllName.AppendFormat(TwoDigExt, keyboardIndex);
		KeyTranslator->ChangeKeyData(keyDataDllName);
		}

    KeyRepeat=new(ELeave) CKeyRepeat(CKeyRepeat::EKeyRepeatPriority);
	TRAPD(r,KeyRepeat->ConstructL());
	if (r!=KErrNone)
		User::Panic(_L("KEYREPEAT"),r);

#ifndef __WINS__
    if (UserSvr::TestBootSequence())
		{
		RDebug::Print(_L("WS_MAIN: TestBootSequence=TRUE, not loading ESHELL.EXE"));
		}
#else
    if (EmulatorAutoRun())
    	{	// don't start ESHELL if we used a self-bootstrapping EXE
    	}
#endif
	else
		{
		RProcess shell;
		r=shell.Create(KShellProcessName, KShellCommandLine);
		__ASSERT_ALWAYS(r==KErrNone,Fault(ECreateShell));
		shell.Resume();
		shell.Close();
		}

	RThread t;
	r=CNotifierSession::NotifierSemaphore.CreateLocal(0);
	if (r!=KErrNone)
		Fault(ECreateNotifierSemaphore);
	r=t.Create(_L("NotifierServer"),NotifierServerThread,KDefaultStackSize,0x2000,0x100000,NULL);
	if (r!=KErrNone)
		Fault(ECreateNotifierThread);
	t.Resume();
	CNotifierSession::NotifierSemaphore.Wait();

	CWsActiveScheduler::Start();
	UserSvr::ReleaseEventHook();
	return(KErrNone);
	}

