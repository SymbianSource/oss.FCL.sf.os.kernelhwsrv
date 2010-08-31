// Copyright (c) 1996-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\window\t_keys.cpp
// 
//
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32hal.h>
#include <e32twin.h>
#include <e32keys.h>
#include <e32svr.h>

const TBool KEY_UP=ETrue;
const TBool KEY_DOWN=EFalse;
const TBool EXPECT_NO_KEY_PRESS=EFalse;
const TBool EXPECT_KEY_PRESS=ETrue;

_LIT(REFKDATA, "REFKDATA");

LOCAL_D RTest test(_L("T_KEYS"));
LOCAL_D CKeyTranslator *KeyTranslator;
LOCAL_D CCaptureKeys *CaptureKeys;

LOCAL_C TBool testType()
//
// Determine whether the Func key sets Alt or Func modifier
//
    {
	TKeyData keyData;
    TBool ret;
    KeyTranslator->TranslateKey(EStdKeyLeftFunc,EFalse,*CaptureKeys,keyData);
    ret=(keyData.iModifiers==(EModifierLeftFunc|EModifierFunc));
    KeyTranslator->TranslateKey(EStdKeyLeftFunc,ETrue,*CaptureKeys,keyData);
    return(ret);
    }

LOCAL_C void testConv(const TDesC& aDes,TBool aKeyup,TBool aRet,TUint aScanCode,TUint aKeyCode,TInt aModifiers)
	{
	TKeyData keyData;
    TBool ret=KeyTranslator->TranslateKey(aScanCode, aKeyup,*CaptureKeys,keyData);
	test.Next(aDes);
	test(ret==aRet);
	test((keyData.iKeyCode==aKeyCode));
	test((keyData.iModifiers==aModifiers));
	}

LOCAL_C void testCtrl(TInt aKeyCode)
//
// Test that prssing Ctrl-xyz returns ascii code xyz
//
    {

    TBuf<16> down;
    TBuf<16> up;

    TInt val[4];
    TInt ii=0;
	TInt key=aKeyCode;

    while(key)
        {
        val[ii++]=key%10;
        key/=10;
        }

	testConv(_L("Left control down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyLeftCtrl,0,EModifierCtrl|EModifierLeftCtrl);

	TUint downModifiers=EModifierCtrl|EModifierLeftCtrl|EModifierPureKeycode;
	TUint upModifiers=EModifierCtrl|EModifierLeftCtrl|EModifierKeyUp;

    while(ii--)
        {
        down.Format(_L("%d down"),val[ii]);
        up.Format(_L("%d up"),val[ii]);
    	testConv(down,KEY_DOWN,EXPECT_NO_KEY_PRESS,val[ii]+'0',0,downModifiers);
    	testConv(up,KEY_UP,EXPECT_NO_KEY_PRESS,val[ii]+'0',0,upModifiers);
		downModifiers&=~(EModifierPureKeycode);
        }

	testConv(_L("Left control up"),KEY_UP,EXPECT_KEY_PRESS,EStdKeyLeftCtrl,aKeyCode,EModifierKeyUp|EModifierSpecial);
    }

/*
LOCAL_C void testAccents(TInt aNum,TInt aLetter,TInt aKeycode)
//
// Check accents by pressing Ctrl-n followed by a letter
//
	{

	TBuf<16> down;
	TBuf<16> up;
//
// Press the Ctrl key
//
	testConv(_L("Left control down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyLeftCtrl,0,EModifierCtrl|EModifierLeftCtrl);
//
// While it's down, press the given number key...
//
    down.Format(_L("%c down"),aNum);
   	testConv(down,KEY_DOWN,EXPECT_NO_KEY_PRESS,aNum,0,EModifierCtrl|EModifierLeftCtrl|EModifierPureKeycode);
//
// Release the number and Ctrl keys
//
    up.Format(_L("%c up"),aNum);
   	testConv(up,KEY_UP,EXPECT_NO_KEY_PRESS,aNum,0,EModifierCtrl|EModifierLeftCtrl|EModifierKeyUp);
	testConv(_L("Left control up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyLeftCtrl,0,EModifierKeyUp);
//
// Press the letter key and check an accent is returned...
//
    down.Format(_L("%c down"),aLetter);
    up.Format(_L("%c up"),aLetter);
   	testConv(down,KEY_DOWN,EXPECT_KEY_PRESS,aLetter,aKeycode,EModifierSpecial);
   	testConv(up,KEY_UP,EXPECT_NO_KEY_PRESS,aLetter,0,EModifierKeyUp);
	}
*/

GLDEF_C TInt E32Main()
//
// Test the keyboard translator
//
    {

    test.Start(_L("Keyboard translator tests"));

    KeyTranslator=CKeyTranslator::New();
    CaptureKeys=new CCaptureKeys();

	CaptureKeys->Construct();


	test.Printf(_L("Load template EKData dll \n"));       // Test with rfkdata.dll	 
	TInt res=KeyTranslator->ChangeKeyData(REFKDATA);
	test_KErrNone(res);
	
	/* Test the AddCapture failure case */
	TCaptureKey ck;
	ck.iModifiers.iMask = 11;
	ck.iModifiers.iValue = 100;
	ck.iKeyCodePattern.iKeyCode = EKeyNull;
	ck.iKeyCodePattern.iPattern = EAnyKey;
	ck.iApp = 111;
	ck.iHandle = 123;

	TInt r = KErrNone;
	TRAP(r, CaptureKeys->AddCaptureKeyL(ck));
	test_Equal(r, KErrArgument);

	/* Test the AddCapture success case */
	ck.iModifiers.iMask = 11;
	ck.iModifiers.iValue = 1;

	TRAP(r, CaptureKeys->AddCaptureKeyL(ck));
	test_KErrNone(r);

	/* Test the SetCapture case */
	TCaptureKey replaceck;
	replaceck.iModifiers.iMask = 0;
	replaceck.iModifiers.iValue = 0;
	replaceck.iKeyCodePattern.iKeyCode = EKeyNull;
	replaceck.iKeyCodePattern.iPattern = EAnyKey;
	replaceck.iApp = 222;
	replaceck.iHandle = 456;

	/* Test the SetCapture failure case */
	CaptureKeys->SetCaptureKey(replaceck.iHandle, ck);

	/* Test the SetCapture success case */
	CaptureKeys->SetCaptureKey(ck.iHandle, replaceck, 0x80);

	/* Test the Cancelcapture failure case */
	CaptureKeys->CancelCaptureKey(ck.iHandle);

	/* Let us add one more with a different set of mask to test ProcessCaptureKeys */
	ck.iModifiers.iMask = 11;
	ck.iModifiers.iValue = 1;
	ck.iKeyCodePattern.iKeyCode = EKeyNull;
	ck.iKeyCodePattern.iPattern = EMatchLeftOrRight+1;
	ck.iApp = 111;
	ck.iHandle = 123;

	TRAP(r, CaptureKeys->AddCaptureKeyL(ck));
	test_KErrNone(r);

	/* Let us add one more with a different set of mask to test ProcessCaptureKeys */
	ck.iModifiers.iMask = 11;
	ck.iModifiers.iValue = 1;
	ck.iKeyCodePattern.iKeyCode = EKeyNull;
	ck.iKeyCodePattern.iPattern = EAnyKey;
	ck.iApp = 333;
	ck.iHandle = 789;

	TRAP(r, CaptureKeys->AddCaptureKeyL(ck));
	test_KErrNone(r);

    TUint scancode=EStdKeyLeftArrow;

//
// Test that the special keys pass through and anything after
// or before raises an error
//
	testConv(_L("First Special key down"),KEY_DOWN,EXPECT_KEY_PRESS,ESpecialKeyBase,ESpecialKeyBase,0);
	testConv(_L("First Special key up"),KEY_UP,EXPECT_NO_KEY_PRESS,ESpecialKeyBase,0,EModifierKeyUp);

	testConv(_L("Last Sepcial key down"),KEY_DOWN,EXPECT_KEY_PRESS,ESpecialKeyBase+ESpecialKeyCount-1,ESpecialKeyBase+ESpecialKeyCount-1,0);
	testConv(_L("Last Special key up"),KEY_UP,EXPECT_NO_KEY_PRESS,ESpecialKeyBase+ESpecialKeyCount-1,0,EModifierKeyUp);

	testConv(_L("Key before Special range down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,ESpecialKeyBase-1,0,0);
	testConv(_L("Key before Special range up"),KEY_UP,EXPECT_NO_KEY_PRESS,0,0,EModifierKeyUp);

	testConv(_L("Key after Special range down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,ESpecialKeyBase+ESpecialKeyCount,0,0);
	testConv(_L("Key after Sepcial range up"),KEY_UP,EXPECT_NO_KEY_PRESS,ESpecialKeyBase+ESpecialKeyCount,0,EModifierKeyUp);
//
// Test a range of keys...
//
	for(scancode='A';scancode<='Z';scancode++)
		{
		TBuf<16> buf;

		buf.Format(_L("Testing key %c"),scancode);
		testConv(buf,KEY_DOWN,EXPECT_KEY_PRESS,scancode,'a'+scancode-'A',EModifierAutorepeatable);
		testConv(buf,KEY_UP,EXPECT_NO_KEY_PRESS,scancode,0,EModifierKeyUp);
		}
//
// Test shift and control modifiers
//
	testConv(_L("Left shift down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyLeftShift,0,EModifierShift|EModifierLeftShift);
	testConv(_L("Left shift up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyLeftShift,0,EModifierKeyUp);

	testConv(_L("Right shift down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyRightShift,0,EModifierShift|EModifierRightShift);
	testConv(_L("Right shift up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyRightShift,0,EModifierKeyUp);

	testConv(_L("Left shift down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyLeftShift,0,EModifierShift|EModifierLeftShift);
	testConv(_L("Right shift down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyRightShift,0,EModifierShift|EModifierRightShift|EModifierLeftShift);
	testConv(_L("Left shift up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyLeftShift,0,EModifierKeyUp|EModifierShift|EModifierRightShift);
	testConv(_L("Right shift up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyRightShift,0,EModifierKeyUp);

	testConv(_L("Left control down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyLeftCtrl,0,EModifierCtrl|EModifierLeftCtrl);
	testConv(_L("Left control up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyLeftCtrl,0,EModifierKeyUp|EModifierPureKeycode);

	testConv(_L("Right control down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyRightCtrl,0,EModifierCtrl|EModifierRightCtrl);
	testConv(_L("Right control up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyRightCtrl,0,EModifierKeyUp|EModifierPureKeycode);

	testConv(_L("Left control down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyLeftCtrl,0,EModifierCtrl|EModifierLeftCtrl);
	testConv(_L("Right control down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyRightCtrl,0,EModifierCtrl|EModifierRightCtrl|EModifierLeftCtrl);
	testConv(_L("Left control up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyLeftCtrl,0,EModifierKeyUp|EModifierCtrl|EModifierRightCtrl|EModifierPureKeycode);
	testConv(_L("Right control up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyRightCtrl,0,EModifierKeyUp|EModifierPureKeycode);
//
// Test function and alt keys - these are swapped on some platforms
//
    if(testType())
        {
    	testConv(_L("Left func down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyLeftFunc,0,EModifierFunc|EModifierLeftFunc);
    	testConv(_L("Left func up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyLeftFunc,0,EModifierKeyUp);

    	testConv(_L("Right func down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyRightFunc,0,EModifierFunc|EModifierRightFunc);
    	testConv(_L("Right func up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyRightFunc,0,EModifierKeyUp);

    	testConv(_L("Left func down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyLeftFunc,0,EModifierFunc|EModifierLeftFunc);
    	testConv(_L("Right func down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyRightFunc,0,EModifierFunc|EModifierRightFunc|EModifierLeftFunc);
    	testConv(_L("Left func up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyLeftFunc,0,EModifierKeyUp|EModifierFunc|EModifierRightFunc);
    	testConv(_L("Right func up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyRightFunc,0,EModifierKeyUp);

    	testConv(_L("Left alt down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyLeftAlt,0,EModifierAlt|EModifierLeftAlt);
    	testConv(_L("Left alt up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyLeftAlt,0,EModifierKeyUp);

    	testConv(_L("Right alt down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyRightAlt,0,EModifierAlt|EModifierRightAlt);
    	testConv(_L("Right alt up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyRightAlt,0,EModifierKeyUp);

    	testConv(_L("Left alt down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyLeftAlt,0,EModifierAlt|EModifierLeftAlt);
    	testConv(_L("Right alt down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyRightAlt,0,EModifierAlt|EModifierRightAlt|EModifierLeftAlt);
    	testConv(_L("Left alt up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyLeftAlt,0,EModifierKeyUp|EModifierAlt|EModifierRightAlt);
    	testConv(_L("Right alt up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyRightAlt,0,EModifierKeyUp);
        }
    else
        {
    	testConv(_L("Left func down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyLeftAlt,0,EModifierFunc|EModifierLeftFunc);
    	testConv(_L("Left func up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyLeftAlt,0,EModifierKeyUp);

    	testConv(_L("Right func down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyRightAlt,0,EModifierFunc|EModifierRightFunc);
    	testConv(_L("Right func up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyRightAlt,0,EModifierKeyUp);

    	testConv(_L("Left func down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyLeftAlt,0,EModifierFunc|EModifierLeftFunc);
    	testConv(_L("Right func down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyRightAlt,0,EModifierFunc|EModifierRightFunc|EModifierLeftFunc);
    	testConv(_L("Left func up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyLeftAlt,0,EModifierKeyUp|EModifierFunc|EModifierRightFunc);
	    testConv(_L("Right func up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyRightAlt,0,EModifierKeyUp);

    	testConv(_L("Left alt down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyLeftFunc,0,EModifierAlt|EModifierLeftAlt);
    	testConv(_L("Left alt up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyLeftFunc,0,EModifierKeyUp);

    	testConv(_L("Right alt down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyRightFunc,0,EModifierAlt|EModifierRightAlt);
    	testConv(_L("Right alt up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyRightFunc,0,EModifierKeyUp);

    	testConv(_L("Left alt down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyLeftFunc,0,EModifierAlt|EModifierLeftAlt);
    	testConv(_L("Right alt down"),KEY_DOWN,EXPECT_NO_KEY_PRESS,EStdKeyRightFunc,0,EModifierAlt|EModifierRightAlt|EModifierLeftAlt);
    	testConv(_L("Left alt up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyLeftFunc,0,EModifierKeyUp|EModifierAlt|EModifierRightAlt);
    	testConv(_L("Right alt up"),KEY_UP,EXPECT_NO_KEY_PRESS,EStdKeyRightFunc,0,EModifierKeyUp);
        }
//
// Test control entering of ascii codes
//
    testCtrl(123);
    testCtrl(217);
    testCtrl(53);
    testCtrl(37);

/* These accent tests are dependent upon the keyboard mapping being used. They need enhancing
   so that they adjust according to the platform on which they are being run

//
// Test that pressing Ctrl-1 followed by 'e' gives accents
//
	testAccents('1','E',0xe6);
	testAccents('2','E',0xea);
	testAccents('3','E',0xe8);
	testAccents('4','E',0xe9);
	testAccents('5','A',0xe3);
	testAccents('6','E',0xea);

*/

	/* Test the CancelAllCaptureKeys failure case */
	CaptureKeys->CancelAllCaptureKeys(ck.iApp);

	/* Test the CancelCaptureKey success case */
	CaptureKeys->CancelCaptureKey(replaceck.iHandle);

	/* Now add a CaptureKey to test CancelAllCaptureKeys success case */
	ck.iModifiers.iMask = 11;
	ck.iModifiers.iValue = 1;
	ck.iKeyCodePattern.iKeyCode = EKeyNull;
	ck.iKeyCodePattern.iPattern = EAnyKey;
	ck.iApp = 111;
	ck.iHandle = 123;

	TRAP(r, CaptureKeys->AddCaptureKeyL(ck));
	test_KErrNone(r);

	/* Test CancelAllCaptureKeys success case */
	CaptureKeys->CancelAllCaptureKeys(ck.iApp);

	delete CaptureKeys;

	test.End();
	return(KErrNone);
    }

