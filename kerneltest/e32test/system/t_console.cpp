// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_console.cpp
// Overview:
// Test methods of CConsoleBase, CProxyConsole and CColorConsole class.
// API Information:
// CConsoleBase.
// Details:
// - Create an object of CTestConsole class which is derived from CColorConsole. 
// - Call methods of CColorConsole class with this test object.
// - Create a full screen console object of CConsoleBase.
// - Call methods of CConsoleBase class with this console object.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32event.h>
#include <e32svr.h>

LOCAL_D RTest test(_L("T_CONSOLE"));

// Literals
_LIT(KTxtTitle,"Console App");
_LIT(KTxtNewTitle,"New Console Title");
_LIT(KTxtWrite,"Write some text ");
_LIT(KTxtPressAnyKey,"Press any key or wait 2 seconds");

// This is test class for testing methods of CColorConsoleBase
class CTestConsole : public CColorConsoleBase
    {
    public:
    TInt Extension_(TUint aExtensionId, TAny*& a0, TAny* a1)
        {
        return CColorConsoleBase::Extension_(aExtensionId, a0, a1);
        }
    };

// Creates an object of CTestConsole.
// Calls methods of CColorConsoleBase 
LOCAL_C void InitTestConsoleL()
    {
    // create a full screen test console object
    CTestConsole* testConsole;
    testConsole = (CTestConsole*)Console::NewL(KTxtTitle, TSize(KConsFullScreen,KConsFullScreen));
        
    CleanupStack::PushL(testConsole);
    
    // Defines the bold text attribute.
    testConsole->SetTextAttribute(ETextAttributeBold);
    testConsole->Write(KTxtWrite);
    // Defines the inverse text attribute.
    testConsole->SetTextAttribute(ETextAttributeInverse);
    testConsole->Write(KTxtWrite);
    // Defines the highlight text attribute.
    testConsole->SetTextAttribute(ETextAttributeHighlight);
    testConsole->Write(KTxtWrite);
    // Defines the normal text attribute.
    testConsole->SetTextAttribute(ETextAttributeNormal);
    testConsole->Write(KTxtWrite);
	    
    TInt a1 = 0;
    TAny* any = 0;
    testConsole->Extension_((TUint)a1,any,any);
    
    // cleanup and return
    CleanupStack::PopAndDestroy(); // close console
    }

LOCAL_C void SimulateKeyPress(TStdScanCode aScanCode)
    {
    TRawEvent eventDown;
    eventDown.Set(TRawEvent::EKeyDown, aScanCode);
    UserSvr::AddEvent(eventDown);
    TRawEvent eventUp;
    eventUp.Set(TRawEvent::EKeyUp, aScanCode);
    UserSvr::AddEvent(eventUp);    
    }

LOCAL_C void ReadConsole(CConsoleBase* aConsole)
    {
    aConsole->Printf(KTxtPressAnyKey);
    
    TRequestStatus keyStatus;
    // Gets a keystroke from the console window, asynchronously
    aConsole->Read(keyStatus);
    RTimer timer;
    test_KErrNone(timer.CreateLocal());
    TRequestStatus timerStatus;
    timer.After(timerStatus,2*1000000);
    User::WaitForRequest(timerStatus,keyStatus);
    if(keyStatus!=KRequestPending)
        {
        TKeyCode keyCode = aConsole->KeyCode();
        aConsole->Printf(_L("Keycode %d\n"),keyCode);
        }
    timer.Cancel();
    // Cancels any outstanding request 
    aConsole->ReadCancel();
    User::WaitForAnyRequest();
    }

LOCAL_C void InitConsoleL()
    {
    // create a full screen console object
    CConsoleBase* console;
    console = Console::NewL(KTxtTitle, TSize(KConsFullScreen,KConsFullScreen));
      
    CleanupStack::PushL(console);
  
    //Gets the size of the console
    TSize screenSize = console->ScreenSize();
    test.Printf(_L("Screen size %d %d\r\n"),screenSize.iWidth,screenSize.iHeight);
  
    // Gets the cursor's x-position
    TInt x = console->WhereX();
    // Gets the cursor's y-position
    TInt y = console->WhereY();
    test_Equal(x, 0);
    test_Equal(y, 0);
    test.Printf(_L("**1** Cursor positions x: %d  y: %d\r\n"),x, y);
  
    // Sets the cursor's x-position
    for(TInt i=0; i<4; i++)
        {
        console->SetPos(screenSize.iWidth + i);
        x = console->WhereX();
        test_Equal(x, screenSize.iWidth -3);
		}
    
    test.Printf(_L("**2** Cursor positions x: %d  y: %d\r\n"),x, y);
      
    // Clears the console and set cursor to position 0,0
    console->ClearScreen();
    test_Equal(console->WhereX(), 0);
	test_Equal(console->WhereY(), 0);
        
    // Sets the cursor's x-position and y-position
    for(TInt j=0; j<4; j++)
        {
        console->SetPos(screenSize.iWidth - j, screenSize.iHeight - j);
        x = console->WhereX();
        y = console->WhereY();
        test_Equal(x, screenSize.iWidth -3);
		test_Equal(y, screenSize.iHeight -3);
		}
    test.Printf(_L("**3** Cursor positions x: %d  y: %d\r\n"),x, y);
     
    console->SetPos(0,0);
    x = console->WhereX();
    y = console->WhereY();
    test_Equal(x, 0);
    test_Equal(y, 0);
	test.Printf(_L("**4** Cursor positions x: %d  y: %d\r\n"),x, y);
  
    console->SetPos(screenSize.iWidth/2,screenSize.iHeight/2);
    x = console->WhereX();
    y = console->WhereY();
    test.Printf(_L("**5** Cursor positions x: %d  y: %d\r\n"),x, y);
  
    // Sets the percentage height of the cursor
    console->SetCursorHeight(50);
  
    // Gets the current cursor position relative to the console window
    TPoint cursorPos = console->CursorPos();
    test.Printf(_L("CursorPos iX: %d  iY: %d\r\n"),cursorPos.iX, cursorPos.iY);
    
    // Puts the cursor at the specified position relative
    // to the current cursor position
    TPoint relPos;
    relPos.iX = screenSize.iWidth/4;
    relPos.iY = screenSize.iHeight/4;
    console->SetCursorPosRel(relPos);
    cursorPos = console->CursorPos();
    test.Printf(_L("CursorPosRel iX: %d  iY: %d\r\n"),cursorPos.iX, cursorPos.iY);
  
    // Puts the cursor at the absolute position in the window
    cursorPos.iX = screenSize.iWidth/6;
    cursorPos.iY = screenSize.iHeight/6;
    console->SetCursorPosAbs(cursorPos);
    cursorPos = console->CursorPos();
    test.Printf(_L("CursorPosAbs iX: %d  iY: %d\r\n"),cursorPos.iX, cursorPos.iY);
  
    // Sets a new console title
    console->SetTitle(KTxtNewTitle);
    // Writes the content of the specified descriptor to the console window
    console->Write(KTxtWrite);
    cursorPos.iX = cursorPos.iX + 6;
    console->SetCursorPosAbs(cursorPos);
    // Clears the console from the current cursor position to the end of the line
    console->ClearToEndOfLine();
    // Clears the console and set cursor to position 0,0
    console->ClearScreen();
  
    TUint keyModifiers = console->KeyModifiers();
	test.Printf(_L("keyModifiers %d"),keyModifiers);
    TKeyCode keyCode = console->KeyCode();
    ReadConsole(console);
  
    SimulateKeyPress(EStdKeyEnter);
    keyCode = console->Getch();
  
    // cleanup and return
    CleanupStack::PopAndDestroy(); // close console
    }

TInt E32Main()
//
// 
//
    {
    test.Title();
    __UHEAP_MARK;

    test.Start(_L("Testing Console"));
    // Get cleanup stack
    CTrapCleanup* cleanup = CTrapCleanup::New();
    TInt result = KErrNoMemory;
    if (cleanup)
	    {
        TRAP(result, InitTestConsoleL());
	    TRAP(result, InitConsoleL());
	    // Destroy the cleanup stack
	    delete cleanup;
	    }
	__UHEAP_MARKEND;
	test.End();
	return(result);
	}

