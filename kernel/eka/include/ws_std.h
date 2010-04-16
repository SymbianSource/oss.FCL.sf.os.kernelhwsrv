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
// e32\include\ws_std.h
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __WS_STD_H__
#define __WS_STD_H__

#include <e32base.h>
#include <e32base_private.h>
#include <e32cons.h>
#include <e32twin.h>
#include <e32ver.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
//
#include <twintnotifier.h>
//
const TInt KW32MajorVersionNumber=1;
const TInt KW32MinorVersionNumber=0;
const TInt KMessageSlots=3;
//

typedef TUint8 TColorIndex;

enum TConsolePanic
	{
	EConsServerFailed,
	EConsNotSupportedYet
	};
//
enum TWsPanic
	{
	EPrematureOperation,
	ETooManyWindowsOpen,
	EWindowTooWide,
	EWindowTooThin,
	EWindowTooHigh,
	EWindowTooShort,
	EWindowOutOfMemory,
	EDoubleReadRequest,
	};
//
enum TWsFault
	{
	ECreateScheduler,
	ECreateServer,
	EStartServer,
	ECreateEvent,
	ECreateShell,
	ESchedulerError,
	EWindowsInitialisation,
	ENoKeyboardTranslator,
	ECreateNotifierSemaphore,
	ECreateNotifierThread,
	ELocaleDll,
	EChangeLocale,
	};
//
struct SWsKey
	{
	TKeyData iKeyData;
    TInt iType;
    TUint8 iPointerNumber;
    TPoint iMousePos;
	TSglQueLink iLink;
	};

struct ColorInformation
	{
	TColorIndex iFg;
	TColorIndex iBg;	
	};
//
class CScreenDriver;
class CWsWindow : public CBase
	{
	friend class CEvent;
	friend class CWsSession;
	friend class CNotifierSession;
    friend class CKeyRepeat;
private:
	enum
		{
		EBackgroundNumber=0,
		EMaxOpenWindows=64,
		ENormalAttribute=7,
		EMouseCharacter=219,
		ECursorPeriodicPriority=2000
		};
public:
	CWsWindow();
	static void New();
	inline static void WaitOnService() {ServiceMutex.Wait();}
	inline static void SignalService() {ServiceMutex.Signal();}
	static TBool RawEventMode();
	static void QueueRawEvent(TRawEvent& anEvent);
	void CreateL(const TSize &aSize);

private:
	~CWsWindow();
	void Display();
	TBool IsTop() const;
	void MakeTopWindow();
	void SetClip();
	void Clear();
	void WriteCharacter(const TText *aCharacter);
	void CarriageReturn();
	void LineFeed();
	void Write(const TDesC &aBuffer);
	void Refresh();
	void SaveEdges();
	void RestoreEdges();
	void SetWiew();
	TBool IsInClippedTextArea(const TPoint& aPoint) const;
	void SetCursor();
	static void TextFill(TText *aBuffer, TInt aLength, const TText *aValue);
	static TInt Offset(const TPoint &aPosition,const TSize &aSize);
	static void RotateWindowsForwards();
	static void RotateWindowsBackwards();
	static void BeginUpdateScreen();
	static void EndUpdateScreen();
	static void DrainAllReadRequests();
    static void ControlInformAllMouse(TBool anIndicator);
#if defined(_UNICODE)
	static TInt IsHankaku(const TText aCode);
	static TInt FitInWidth(TText* aDest,TInt aWidth,TInt aAsciiCol,TText aCode);
	static TInt OffsetHZa(const TText* aDest,const TPoint& aPosition,const TSize& aSize,TInt& aX);
	static TInt OffsetHZwP(const TText* aDest,const TPoint& aPosition,const TSize& aSize,TPoint& aP);
	static TInt OffsetHZ(const TText* aDest,const TPoint& aPosition,const TSize& aSize);
	static TText GetCharFromOffset(const TText* aDest,const TPoint& aPosition,const TSize& aSize);
	static TText *GetCpFromOffset(const TText* aDest,const TPoint& aPosition,const TSize& aSize);
#endif
	void ScrollUp();
	void Left();
	void Right();
	void FormFeed();
	void BackSpace();
	void HorizontalTab();
	TBool IsRectVisible(TRect& aRect) const;
	void SetFrame();
	void DrainReadRequest();
	TBool EnqueReadRequest(const RMessage2& aMessage);
	void DequeReadRequest();
	void InformMouse(TPoint aPos);
	void QueueWindowKey(TKeyData &aKeystroke);
	void DoMouseLeftButton();
	void ControlMaximised(TBool anIndicator);
	void ControlOnTop(TBool anIndicator);
	static void Delete();
	static CWsWindow *TopWindow();
	static CWsWindow *BottomWindow();
	static TInt8 NewNumberL();
	static void ReleaseNumber(TInt8 aNumber);
	static void Redraw();
	static void KeyPress(TKeyData& aKeystroke);
	static void QueueTopWindowKey(TKeyData& aKeystroke);
	static void InformTopMouse(TPoint aPos);
	static TInt ChangeTopWindowSize(TSize aGrowth);
	static TInt SlideTopWindowRelative(TPoint aDirection);
	static TInt MoveTopWindowRelative(TPoint aDirection);
	static void ControlTopWindowMaximised(TBool anIndicator);
	static TInt FlashCursor(TAny *aParameter);
	static void ResetVisibilityMap();
	static void UpdateScreen(TPoint &aPosition,TInt aLength,TInt8 aNumber,TText *aTextBuffer,ColorInformation *anAttributeBuffer);
	static void Background();
	static void TurnMouseOff();
	static void TurnMouseOn();
	static void MouseMove(TPoint aGraphicsPosition);
	static void MouseLeftButton();
	static void MouseLeftButtonUp();
	static CWsWindow *MouseWindow();
	static void ChangeUIColors();
	static TInt SetMode(TVideoMode aMode);
	TSize Size();
	TPoint CursorPosition();
	void WriteDone();
	void SetView();
	void SetFull();
	void ClearToEndOfLine();
	void NewLine();
	void SetCursorHeight(TInt aPercentage);
	void SetTitle(const TDesC &aName);
	void SetSize(const TSize &aSize); 
	void SetWindowPosAbs(const TPoint &aPosition);
	void SetCursorPosAbs(const TPoint &aPosition);
	void SetCursorPosRel(const TPoint &aPosition);
	void ControlScrollBars(TBool anIndicator);
	void ControlWrapLock(TBool anIndicator);
    void ControlPointerEvents(TBool anIndicator);
	void ControlScrollLock(TBool anIndicator);
	void ControlVisibility(TBool anIndicator);
	void ControlAllowResize(TBool anIndicator);
	void ControlCursorRequired(TBool anIndicator);
	void ControlNewLineMode(TBool anIndicator);
	void ControlRawEventMode(TBool anIndicator);
    void QueueWindowRawEvent(TRawEvent& anEvent);
	void MouseSlide();
	void SetTextAttribute(TTextAttribute anAttribute);

private:
	TInt8 iNumber;
	TSize iCurrentSize;
	TSize iClippedSize;
	TBool iIsVisible;
	TPoint iViewOrigin;
	TSize iViewSize;
	TPoint iCurrentOffset;
	TText *iTextBuffer;
	ColorInformation *iAttributeBuffer;
    TUint8 iFillAttribute;
	TBool iCursorRequired;
	TBool iCursorIsOn;
	TPoint iCursorPos;
	TPoint iLastCursorPos;
	TText iCursor;
	TBool iScrollLock;
    TBool iWrapLock;
	TBool iNewLineMode;
	TBool iOnTop;
	TBool iAllowResize;
	TBool iAllowSlide;
	TBool iReadIsValid;
	TDblQueLink iLink;
    static TSize ScreenSize;
	static CScreenDriver *ScreenDriver;
	static TDblQue<CWsWindow> WQueue;
	static TInt8 *VisibilityMap;
	static TPoint MousePos;
	static TSize FontSize;
	static CBitMapAllocator *Numbers;
	static CPeriodic *CursorPeriodic;
	static TText *BlankLineText;
	static ColorInformation *BlankLineAttributes;
	static TBool MouseIsCaptured;
	static RMutex MouseMutex;
	static RMutex ServiceMutex;
	static TInt Count;
	static const TText Cursors[101];
	static CWsWindow* RawEventWindow;
	static TPoint ScrollWithMouse;
	static TPoint MoveWithMouse;
	static TPoint ResizeWithMouse;
	static TInt ScrollSpeed;
	static TColorIndex ScreenColor;
	static TColorIndex WindowBgColor;
	static TColorIndex BorderColor;
	static TColorIndex IndexOf[8];
	TSglQue<SWsKey> iKQueue;
	RMessage2 iReadRequest;
	TPoint iMaximumOrigin;
	TSize iMaximumSize;
	TPoint iMinimumOrigin;
	TSize iMinimumSize;
    TBool iHasScrollBars;
    TBool iPointerEvents;
    TFileName iTitle;
	RMessage2 iMessage;
	TColorIndex iFgColor;
	TColorIndex iBgColor;
	};

class CWsSession : public CSession2
	{
public:
	enum
		{
		EConsoleCreate,
		EConsoleSet,
		EConsoleClearScreen,
		EConsoleClearToEndOfLine,
		EConsoleSetWindowPosAbs,
		EConsoleSetCursorHeight,
		EConsoleSetCursorPosAbs,
		EConsoleSetCursorPosRel,
		EConsoleCursorPos,
		EConsoleControl,
		EConsoleWrite,
		EConsoleRead,
		EConsoleReadCancel,
		EConsoleDestroy,
		EConsoleSetTitle,
		EConsoleSetSize,
		EConsoleSize,
		EConsoleScreenSize,
		EConsoleSetMode,
		EConsoleSetPaletteEntry,
		EConsoleGetPaletteEntry,
		EConsoleSetTextColors,
		EConsoleSetUIColors,
		EConsoleSetTextAttribute
		};
public:
	CWsSession();
	~CWsSession();
	void Attach(CWsWindow* aWindow);
	virtual void ServiceL(const RMessage2& aMessage);
	virtual void ServiceError(const RMessage2& aMessage,TInt aError);
private:
	CWsWindow* iWindow;
	RMessagePtr2 iCurMsg;
	TInt iTestFast;
	};
//
class CWsServer : public CServer2
	{
public:
	enum {EPriority=1000};
public:
	static void New();
	~CWsServer();
	virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const;
private:
	CWsServer(TInt aPriority);
	};
//
class CKeyRepeat : public CTimer
    {
public:
    enum { EKeyRepeatPriority=1990 };

    CKeyRepeat(TInt aPriority);
    void ConstructL();
    void Request(TKeyData& aKeyData);
    virtual void RunL();
    void SetRepeatTime(TInt aDelay,TInt aRate);
    void RepeatTime(TInt& aDelay,TInt& aRate);

    TKeyData iKeyData;
private:
    enum { EDefaultKeyRepeatDelay=500000, EDefaultKeyRepeatRate=30000 };

    TInt iDelay;
    TInt iRate;
    };
//
class CEvent : public CActive
	{
public:
	enum {EPriority=2000};
public:
	static void New();
	~CEvent();
	void Request();
	virtual void DoCancel();
	virtual void RunL();
protected:
	CEvent(TInt aPriority);
private:
	TRawEventBuf iEvent;
	static CCaptureKeys *CaptureKeys;
    TInt iRepeatScanCode;
	};
//
class CWsActiveScheduler : public CActiveScheduler
	{
public:
	static void New();
	virtual void Error(TInt anError) const;
	};
//
#include "w32disp.h"
//
GLREF_C TInt WindowServerThread(TAny *anArg);
GLREF_C void Panic(TWsPanic aPanic);
GLREF_C void Fault(TWsFault aFault);
//
IMPORT_C void Panic(TConsolePanic aPanic);

#endif	// __WS_STD_H__
