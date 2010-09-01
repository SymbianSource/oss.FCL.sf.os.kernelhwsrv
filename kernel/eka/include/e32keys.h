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
// e32\include\e32keys.h
// 
//

#ifndef __E32KEYS_H__
#define __E32KEYS_H__

// Using #define instead of const TInt so this file is parsable by rcomp

/**
@publishedPartner
@released

Base code of range that doesn't correspond to a character
*/
#define ENonCharacterKeyBase 0xf800

/**
@publishedPartner
@released

Size of range that doesn't correspond to a character
*/
#define ENonCharacterKeyCount 0x100

/**
@publishedPartner
@released

Base code of range that is reserved for application framework use and guaranteed
not to be produced by any keyboard.
*/
#define ESpecialKeyBase 0xf700

/**
@publishedPartner
@released

Size of range that is reserved for application framework use and guaranteed
not to be produced by any keyboard.
*/
#define ESpecialKeyCount 0x100




/**
@publishedAll
@released

Modifier flags for a key or pointer event.

The modifier flags for a key event are held in TKeyEvent::iModifiers.

The modifier flags for a pointer event are held in TPointerEvent::iModifiers.

@see TKeyEvent::iModifiers
@see TPointerEvent::iModifiers
*/
enum TEventModifier
	{
	EModifierAutorepeatable=0x00000001, /**< Key event can auto-repeat.*/
	EModifierKeypad=0x00000002,         /**< The key that generated the event was on the numeric keypad, on the emulator.*/
	EModifierLeftAlt=0x00000004,        /**< Left Alt key.*/
	EModifierRightAlt=0x00000008,       /**< Right Alt key.*/
	EModifierAlt=0x00000010,            /**< Single Alt key.*/
	EModifierLeftCtrl=0x00000020,       /**< Left Control (Ctrl) key.*/
	EModifierRightCtrl=0x00000040,      /**< Right Control (Ctrl) key.*/
	EModifierCtrl=0x00000080,           /**< Single Control (Ctrl) key.*/
	EModifierLeftShift=0x00000100,      /**< Left Shift key.*/
	EModifierRightShift=0x00000200,     /**< Right Shift key.*/
	EModifierShift=0x00000400,          /**< Single Shift key.*/
	EModifierLeftFunc=0x00000800,       /**< Left Fn key.*/
	EModifierRightFunc=0x00001000,      /**< Right Fn key.*/
	EModifierFunc=0x00002000,           /**< Single Fn key.*/
	EModifierCapsLock=0x00004000,       /**< Caps lock key.*/
	EModifierNumLock=0x00008000,        /**< Num lock key.*/
	EModifierScrollLock=0x00010000,     /**< Scroll lock key.*/
	EModifierKeyUp=0x00020000,          /**< Key up event.*/
	EModifierSpecial=0x00040000,        /**< The keycode is a non-standard keyboard character that has been generated in a special keyboard mode, for example accented vowels.*/
	EModifierDoubleClick=0x00080000,    /**< Double click.*/
    EModifierPureKeycode=0x00100000,    /**< The key code in the key event is not changed. E.g.an alphabetic key is not changed by the Caps Lock or Shift key being pressed.*/
	EModifierKeyboardExtend=0x00200000,	/**< The "Keyboard extend" generated modifier. */
	EModifierCancelRotation=0x00000000, /**< No Keyboard rotation is in effect. */
	EModifierRotateBy90=0x00400000,		/**< Keyboard rotation through 90 degrees clockwise is in effect. */
	EModifierRotateBy180=0x00800000,	/**< Keyboard rotation through 180 degrees clockwise is in effect. */
	EModifierRotateBy270=0x01000000,	/**< Keyboard rotation through 270 degrees clockwise is in effect. */
	EModifierPointer3DButton1=0x02000000,/**< 3D pointer device specific modifier (button 1). */
	EModifierPointer3DButton2=0x04000000,/**< 3D pointer device specific modifier (button 2). */
	EModifierPointer3DButton3=0x08000000,/**< 3D pointer device specific modifier (button 3). */
	EModifierAdvancedPointerEvent=0x10000000, /**< TPointerEvent is a TAdvancedPointerEvent.*/
	EAllModifiers=0x1fffffff            /**< A combination of all event modifiers.*/
	};




/**
@publishedAll
@released

Specifies the state of an event modifier, for instance Caps Lock or Num Lock.

Event modifiers are enumerated in TEventModifier.
The modifier state can be set using RWsSession::SetModifierState().

@see TEventModifier
@see RWsSession::SetModifierState()
*/
enum TModifierState
    {
    ETurnOnModifier=0x40, /**< Switch on modifier.*/
    ETurnOffModifier,     /**< Switch off modifier.*/
    EToggleModifier       /**< Toggle the modifier on or off.*/
    };




/**
@publishedAll
@released

Scan codes for the physical keys found on keyboards.

When processing a TKeyEvent, the TStdScanCode in TKeyEvent::iScanCode should
usually be ignored in favour of the TKeyCode in TKeyEvent::iCode.

Using iScanCode would bypass the keyboard mapping and any FEP that happens
to be installed. The exceptions to this general rule are games where
the positions of the keys are more important than their translations,
and FEPs that are implementing keyboard maps themselves. In these cases,
if the iCode is used rather than iScanCode to determine the key pressed,
there will be two unfortunate consequences.
First, the low-level keyboard mapping might re-arrange the mapping that
you are trying to impose.
Second, you will subvert the CTRL+number method of entering Unicode literals.

@see TKeyEvent
@see TStdScanCode
*/
enum TStdScanCode
	{
	EStdKeyNull=0x00,                     /**< No key present. */
	EStdKeyBackspace=0x01,                /**< Scan code for Backspace key.*/
	EStdKeyTab=0x02,                      /**< Scan code for Tab key. */
	EStdKeyEnter=0x03,                    /**< Scan code for Enter key.*/
	EStdKeyEscape=0x04,                   /**< Scan code for Escape (Esc) key.*/
	EStdKeySpace=0x05,                    /**< Scan code for Space key.*/
	EStdKeyPrintScreen=0x06,              /**< Scan code for Print Screen key.*/
	EStdKeyPause=0x07,                    /**< Scan code for Pause key.*/
	EStdKeyHome=0x08,                     /**< Scan code for Home key.*/
	EStdKeyEnd=0x09,                      /**< Scan code for End key.*/
	EStdKeyPageUp=0x0a,                   /**< Scan code for Page Up key.*/
	EStdKeyPageDown=0x0b,                 /**< Scan code for Page Down key.*/
	EStdKeyInsert=0x0c,                   /**< Scan code for Insert key.*/
	EStdKeyDelete=0x0d,                   /**< Scan code for Delete (Del) key.*/
	EStdKeyLeftArrow=0x0e,                /**< Scan code for Left arrow key.*/
	EStdKeyRightArrow=0x0f,               /**< Scan code for Right arrow key.*/
	EStdKeyUpArrow=0x10,                  /**< Scan code for Up arrow key.*/
	EStdKeyDownArrow=0x11,                /**< Scan code for Down arrow key.*/
	EStdKeyLeftShift=0x12,                /**< Scan code for left Shift key.*/
	EStdKeyRightShift=0x13,               /**< Scan code for right Shift key.*/
	EStdKeyLeftAlt=0x14,                  /**< Scan code for left Alt key.*/
	EStdKeyRightAlt=0x15,                 /**< Scan code for right Alt key.*/
	EStdKeyLeftCtrl=0x16,                 /**< Scan code for left Control (Ctrl) key.*/
	EStdKeyRightCtrl=0x17,                /**< Scan code for right Control (Ctrl) key.*/
	EStdKeyLeftFunc=0x18,                 /**< Scan code for left Fn key.*/
	EStdKeyRightFunc=0x19,                /**< Scan code for right Fn key.*/
	EStdKeyCapsLock=0x1a,                 /**< Scan code for Caps lock key.*/
	EStdKeyNumLock=0x1b,                  /**< Scan code for Num lock key.*/
	EStdKeyScrollLock=0x1c,               /**< Scan code for Scroll lock key.*/
	EStdKeyF1=0x60,                       /**< Scan code for function key F1.*/
	EStdKeyF2=0x61,                       /**< Scan code for function key F2.*/
	EStdKeyF3=0x62,                       /**< Scan code for function key F3.*/
	EStdKeyF4=0x63,                       /**< Scan code for function key F4.*/
	EStdKeyF5=0x64,                       /**< Scan code for function key F5*/
	EStdKeyF6=0x65,                       /**< Scan code for function key F6*/
	EStdKeyF7=0x66,                       /**< Scan code for function key F7*/
	EStdKeyF8=0x67,                       /**< Scan code for function key F8*/
	EStdKeyF9=0x68,                       /**< Scan code for function key F9*/
	EStdKeyF10=0x69,                      /**< Scan code for function key F10*/
	EStdKeyF11=0x6a,                      /**< Scan code for function key F11*/
	EStdKeyF12=0x6b,                      /**< Scan code for function key F12*/
	EStdKeyF13=0x6c,                      /**< Scan code for function key F13*/
	EStdKeyF14=0x6d,                      /**< Scan code for function key F14*/
	EStdKeyF15=0x6e,                      /**< Scan code for function key F15*/
	EStdKeyF16=0x6f,                      /**< Scan code for function key F16*/
	EStdKeyF17=0x70,                      /**< Scan code for function key F17*/
	EStdKeyF18=0x71,                      /**< Scan code for function key F18*/
	EStdKeyF19=0x72,                      /**< Scan code for function key F19*/
	EStdKeyF20=0x73,                      /**< Scan code for function key F20*/
	EStdKeyF21=0x74,                      /**< Scan code for function key F21*/
	EStdKeyF22=0x75,                      /**< Scan code for function key F22.*/
	EStdKeyF23=0x76,                      /**< Scan code for function key F23.*/
	EStdKeyF24=0x77,                      /**< Scan code for function key F24.*/
	EStdKeyXXX=0x78,                      /**< Scan code for the key to the left of the 1 key on a standard keyboard.*/
	EStdKeyComma=0x79,                    /**< Scan code for Comma (,) key.*/
	EStdKeyFullStop=0x7a,                 /**< Scan code for Full stop (.) key.*/
	EStdKeyForwardSlash=0x7b,             /**< Scan code for Forward slash (/) key.*/
	EStdKeyBackSlash=0x7c,                /**< Scan code for Back slash (\) key.*/
	EStdKeySemiColon=0x7d,                /**< Scan code for Semi colon (;) key.*/
	EStdKeySingleQuote=0x7e,              /**< Scan code for Single quote (') key.*/
	EStdKeyHash=0x7f,                     /**< Scan code for Hash key (#) key.*/
	EStdKeySquareBracketLeft=0x80,        /**< Scan code for left Square bracket ([) key.*/
	EStdKeySquareBracketRight=0x81,       /**< Scan code for right Square bracket (]) key.*/
	EStdKeyMinus=0x82,                    /**< Scan code for Minus key (-) key.*/
	EStdKeyEquals=0x83,                   /**< Scan code for Equals key (=) key.*/
	EStdKeyNkpForwardSlash=0x84,          /**< Scan code for forward slash (/) key on the Numeric keypad.*/
	EStdKeyNkpAsterisk=0x85,              /**< Scan code for Asterisk (*) key on the Numeric keypad.*/
	EStdKeyNkpMinus=0x86,                 /**< Scan code for Minus (-) key on the Numeric keypad.*/
	EStdKeyNkpPlus=0x87,                  /**< Scan code for Plus (+) key on the Numeric keypad.*/
	EStdKeyNkpEnter=0x88,                 /**< Scan code for Enter key on the Numeric keypad.*/
	EStdKeyNkp1=0x89,                     /**< Scan code for the 1 key on the Numeric keypad.*/
	EStdKeyNkp2=0x8a,                     /**< Scan code for the 2 key on the Numeric keypad.*/
	EStdKeyNkp3=0x8b,                     /**< Scan code for the 3 key on the Numeric keypad.*/
	EStdKeyNkp4=0x8c,                     /**< Scan code for the 4 key on the Numeric keypad.*/
	EStdKeyNkp5=0x8d,                     /**< Scan code for the 5 key on the Numeric keypad.*/
	EStdKeyNkp6=0x8e,                     /**< Scan code for the 6 key on the Numeric keypad.*/
	EStdKeyNkp7=0x8f,                     /**< Scan code for the 7 key on the Numeric keypad.*/
	EStdKeyNkp8=0x90,                     /**< Scan code for the 8 key on the Numeric keypad.*/ 
	EStdKeyNkp9=0x91,                     /**< Scan code for the 9 key on the Numeric keypad.*/
	EStdKeyNkp0=0x92,                     /**< Scan code for the 0 key on the Numeric keypad.*/
	EStdKeyNkpFullStop=0x93,              /**< Scan code for Full stop (.) key on the Numeric keypad.*/
    EStdKeyMenu=0x94,                     /**< Scan code for Menu key.*/
    EStdKeyBacklightOn=0x95,              /**< Scan code for Backlight on key.*/
    EStdKeyBacklightOff=0x96,             /**< Scan code for Backlight off key.*/
    EStdKeyBacklightToggle=0x97,          /**< Scan code for Backlight toggle key.*/
    EStdKeyIncContrast=0x98,              /**< Scan code for Increase contrast key.*/
    EStdKeyDecContrast=0x99,              /**< Scan code for Decrease contrast key.*/
    EStdKeySliderDown=0x9a,               /**< Scan code for Slider down key.*/
    EStdKeySliderUp=0x9b,                 /**< Scan code for Slider up key.*/
    EStdKeyDictaphonePlay=0x9c,           /**< Scan code for Dictaphone play key.*/
    EStdKeyDictaphoneStop=0x9d,           /**< Scan code for Dictaphone stop key.*/
    EStdKeyDictaphoneRecord=0x9e,         /**< Scan code for Dictaphone record key.*/
    EStdKeyHelp=0x9f,                     /**< Scan code for Help key */
    EStdKeyOff=0xa0,                      /**< Scan code for Off key.*/
    EStdKeyDial=0xa1,                     /**< Scan code for Dial key.*/
    EStdKeyIncVolume=0xa2,                /**< Scan code for Increase volume key.*/
    EStdKeyDecVolume=0xa3,                /**< Scan code for Decrease volume key.*/
    EStdKeyDevice0=0xa4,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice1=0xa5,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice2=0xa6,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice3=0xa7,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice4=0xa8,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice5=0xa9,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice6=0xaa,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice7=0xab,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice8=0xac,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice9=0xad,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDeviceA=0xae,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDeviceB=0xaf,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDeviceC=0xb0,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDeviceD=0xb1,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDeviceE=0xb2,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDeviceF=0xb3,                  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyApplication0=0xb4,             /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication1=0xb5,             /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication2=0xb6,             /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication3=0xb7,             /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication4=0xb8,             /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication5=0xb9,             /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication6=0xba,             /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication7=0xbb,             /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication8=0xbc,             /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication9=0xbd,             /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplicationA=0xbe,             /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplicationB=0xbf,             /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplicationC=0xc0,             /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplicationD=0xc1,             /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplicationE=0xc2,             /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplicationF=0xc3,             /**< Scan code for device-specific application launcher key.*/
	EStdKeyYes=0xc4,                      /**< Scan code for Yes key.*/
	EStdKeyNo=0xc5,                       /**< Scan code for No key.*/
	EStdKeyIncBrightness=0xc6,            /**< Scan code for Increase brightness key.*/
	EStdKeyDecBrightness=0xc7,            /**< Scan code for Decrease brightness key.*/
	EStdKeyKeyboardExtend=0xc8,           /**< Scan code for flip actuated when keypad extends to full keyboard.*/
    EStdKeyDevice10=0xc9,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice11=0xca,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice12=0xcb,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice13=0xcc,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice14=0xcd,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice15=0xce,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice16=0xcf,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice17=0xd0,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice18=0xd1,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice19=0xd2,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice1A=0xd3,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice1B=0xd4,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice1C=0xd5,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice1D=0xd6,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice1E=0xd7,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyDevice1F=0xd8,                 /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
    EStdKeyApplication10=0xd9,            /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication11=0xda,            /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication12=0xdb,            /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication13=0xdc,            /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication14=0xdd,            /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication15=0xde,            /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication16=0xdf,            /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication17=0xe0,            /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication18=0xe1,            /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication19=0xe2,            /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication1A=0xe3,            /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication1B=0xe4,            /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication1C=0xe5,            /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication1D=0xe6,            /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication1E=0xe7,            /**< Scan code for device-specific application launcher key.*/
    EStdKeyApplication1F=0xe8,            /**< Scan code for device-specific application launcher key.*/
	EStdKeyDevice20=0xe9,				  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
	EStdKeyDevice21=0xea,				  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
	EStdKeyDevice22=0xeb,				  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
	EStdKeyDevice23=0xec,				  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
	EStdKeyDevice24=0xed,				  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
	EStdKeyDevice25=0xee,				  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
	EStdKeyDevice26=0xef,				  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
	EStdKeyDevice27=0xf0,				  /**< Device-specific scan code - usually represents an extra hardware key on the phone.*/
	EStdKeyApplication20=0xf1,			  /**< Scan code for device-specific application launcher key.*/
	EStdKeyApplication21=0xf2,			  /**< Scan code for device-specific application launcher key.*/
	EStdKeyApplication22=0xf3,			  /**< Scan code for device-specific application launcher key.*/
	EStdKeyApplication23=0xf4,			  /**< Scan code for device-specific application launcher key.*/
	EStdKeyApplication24=0xf5,			  /**< Scan code for device-specific application launcher key.*/
	EStdKeyApplication25=0xf6,			  /**< Scan code for device-specific application launcher key.*/
	EStdKeyApplication26=0xf7,			  /**< Scan code for device-specific application launcher key.*/
	EStdKeyApplication27=0xf8			  /**< Scan code for device-specific application launcher key.*/
	};




/**
@publishedAll
@released
	
Codes generated by non-ASCII key presses.

A key event's character code is held in TKeyEvent::iCode.

@see TKeyEvent::iCode
*/
enum TKeyCode
	{
	EKeyNull=0x0000,                     /**< Code for the Null key. */
	EKeyBell=0x0007,                     /**< Code for the Bell key*/
	EKeyBackspace=0x0008,                /**< Code for the Backspace key */
	EKeyTab=0x0009,                      /**< Code for the Tab key*/
	EKeyLineFeed=0x000a,                 /**< Code for the Linefeed key*/
	EKeyVerticalTab=0x000b,              /**< Code for the Vertical tab key*/
	EKeyFormFeed=0x000c,                 /**< Code for the Form feed key*/
	EKeyEnter=0x000d,                    /**< Code for the Enter key */
	EKeyEscape=0x001b,                   /**< Code for the Escape key */
	EKeySpace=0x0020,                    /**< Code for the Space key*/
	EKeyDelete=0x007f,                   /**< Code for the Delete (Del) key*/
	EKeyPrintScreen=ENonCharacterKeyBase,/**< Code for the Print screen key */
	EKeyPause,                           /**< Code for the Pause key*/
	EKeyHome,                            /**< Code for the Home key*/
	EKeyEnd,                             /**< Code for the End key*/
	EKeyPageUp,                          /**< Code for the Page up key*/
	EKeyPageDown,                        /**< Code for the Page down key*/
	EKeyInsert,                          /**< Code for the Insert key*/
	EKeyLeftArrow,                       /**< Code for the Left arrow key*/
	EKeyRightArrow,                      /**< Code for the Right arrow key*/
	EKeyUpArrow,                         /**< Code for the Up arrow key*/
	EKeyDownArrow,                       /**< Code for the Down arrow key*/
	EKeyLeftShift,                       /**< Code for the left Shift key*/
	EKeyRightShift,                      /**< Code for the right Shift key*/
	EKeyLeftAlt,                         /**< Code for the left Alt key*/
	EKeyRightAlt,                        /**< Code for the right Alt key*/
	EKeyLeftCtrl,                        /**< Code for the left Control (Ctrl) key*/
	EKeyRightCtrl,                       /**< Code for the right Control (Ctrl) key.*/
	EKeyLeftFunc,                        /**< Code for the left Fn key.*/
	EKeyRightFunc,                       /**< Code for the right Fn key.*/
	EKeyCapsLock,                        /**< Code for the Caps lock key.*/
	EKeyNumLock,                         /**< Code for the Num lock key.*/
	EKeyScrollLock,                      /**< Code for the Scroll lock key.*/
	EKeyF1,                              /**< Code for the F1 function key.*/
	EKeyF2,                              /**< Code for the F2 function key.*/
	EKeyF3,                              /**< Code for the F3 function key.*/
	EKeyF4,                              /**< Code for the F4 function key.*/
	EKeyF5,                              /**< Code for the F5 function key.*/
	EKeyF6,                              /**< Code for the F6 function key.*/
	EKeyF7,                              /**< Code for the F7 function key.*/
	EKeyF8,                              /**< Code for the F8 function key.*/
	EKeyF9,                              /**< Code for the F9 function key.*/
	EKeyF10,                             /**< Code for the F10 function key.*/
	EKeyF11,                             /**< Code for the F11 function key.*/
	EKeyF12,                             /**< Code for the F12 function key.*/
	EKeyF13,                             /**< Code for the F13 function key.*/
	EKeyF14,                             /**< Code for the F14 function key.*/
	EKeyF15,                             /**< Code for the F15 function key.*/
	EKeyF16,                             /**< Code for the F16 function key.*/
	EKeyF17,                             /**< Code for the F17 function key.*/
	EKeyF18,                             /**< Code for the F18 function key.*/
	EKeyF19,                             /**< Code for the F19 function key.*/
	EKeyF20,                             /**< Code for the F20 function key.*/
	EKeyF21,                             /**< Code for the F21 function key.*/
	EKeyF22,                             /**< Code for the F22 function key.*/
	EKeyF23,                             /**< Code for the F23 function key.*/
	EKeyF24,                             /**< Code for the F24 function key.*/
    EKeyOff,                             /**< Code for the Off key.*/
    EKeyIncContrast,                     /**< Code for the Increase contrast key.*/
    EKeyDecContrast,                     /**< Code for the Decrease contrast key.*/
    EKeyBacklightOn,                     /**< Code for the Backlight on key.*/
    EKeyBacklightOff,                    /**< Code for the Backlight off key.*/
    EKeyBacklightToggle,                 /**< Code for the Backlight toggle key.*/
    EKeySliderDown,                      /**< Code for the Slider down key.*/
    EKeySliderUp,                        /**< Code for the Slider up key.*/
    EKeyMenu,                            /**< Code for the Menu key.*/
    EKeyDictaphonePlay,                  /**< Code for the Dictaphone play key.*/
    EKeyDictaphoneStop,                  /**< Code for the Dictaphone stop key.*/
    EKeyDictaphoneRecord,                /**< Code for the Dictaphone record key.*/
    EKeyHelp,                            /**< Code for the Help key.*/
    EKeyDial,                            /**< Code for the Dial key.*/
	EKeyScreenDimension0,                /**< Code for the first Screen dimension change key.*/
	EKeyScreenDimension1,                /**< Code for the second Screen dimension change key.*/
	EKeyScreenDimension2,                /**< Code for the third Screen dimension change key.*/
	EKeyScreenDimension3,                /**< Code for the fourth Screen dimension change key.*/
	EKeyIncVolume,                       /**< Code for the increase colume key.*/
	EKeyDecVolume,                       /**< Code for the decrease volume key.*/
	EKeyDevice0,                         /**< Code for a device specific key.*/
	EKeyDevice1,                         /**< Code for a device specific key.*/
	EKeyDevice2,                         /**< Code for a device specific key. */
	EKeyDevice3,                         /**< Code for a device specific key.*/
	EKeyDevice4,                         /**< Code for a device specific key.*/
	EKeyDevice5,                         /**< Code for a device specific key.*/
	EKeyDevice6,                         /**< Code for a device specific key.*/
	EKeyDevice7,                         /**< Code for a device specific key.*/
	EKeyDevice8,                         /**< Code for a device specific key.*/
	EKeyDevice9,                         /**< Code for a device specific key.*/
	EKeyDeviceA,                         /**< Code for a device specific key.*/
	EKeyDeviceB,                         /**< Code for a device specific key.*/
	EKeyDeviceC,                         /**< Code for a device specific key.*/
	EKeyDeviceD,                         /**< Code for a device specific key.*/
	EKeyDeviceE,                         /**< Code for a device specific key.*/
	EKeyDeviceF,                         /**< Code for a device specific key.*/
	EKeyApplication0,                    /**< Code for an Application launcher key.*/
	EKeyApplication1,                    /**< Code for an Application launcher key.*/
	EKeyApplication2,                    /**< Code for an Application launcher key.*/
	EKeyApplication3,                    /**< Code for an Application launcher key.*/
	EKeyApplication4,                    /**< Code for an Application launcher key.*/
	EKeyApplication5,                    /**< Code for an Application launcher key.*/
	EKeyApplication6,                    /**< Code for an Application launcher key.*/
	EKeyApplication7,                    /**< Code for an Application launcher key.*/
	EKeyApplication8,                    /**< Code for an Application launcher key.*/
	EKeyApplication9,                    /**< Code for an Application launcher key.*/
	EKeyApplicationA,                    /**< Code for an Application launcher key.*/
	EKeyApplicationB,                    /**< Code for an Application launcher key.*/
	EKeyApplicationC,                    /**< Code for an Application launcher key.*/
	EKeyApplicationD,                    /**< Code for an Application launcher key.*/
	EKeyApplicationE,                    /**< Code for an Application launcher key.*/
	EKeyApplicationF,                    /**< Code for an Application launcher key.*/
	EKeyYes,                             /**< Code for the Yes key.*/
	EKeyNo,                              /**< Code for the No key.*/
	EKeyIncBrightness,                   /**< Code for the increase brightness key.*/
	EKeyDecBrightness,                   /**< Code for the decrease brightness key. */
	EKeyKeyboardExtend,                  /**< Code for flip actuated when keypad extends to full keyboard.*/
	EKeyDevice10,                        /**< Code for a device specific key.*/
	EKeyDevice11,                        /**< Code for a device specific key.*/
	EKeyDevice12,                        /**< Code for a device specific key. */
	EKeyDevice13,                        /**< Code for a device specific key.*/
	EKeyDevice14,                        /**< Code for a device specific key.*/
	EKeyDevice15,                        /**< Code for a device specific key.*/
	EKeyDevice16,                        /**< Code for a device specific key.*/
	EKeyDevice17,                        /**< Code for a device specific key.*/
	EKeyDevice18,                        /**< Code for a device specific key.*/
	EKeyDevice19,                        /**< Code for a device specific key.*/
	EKeyDevice1A,                        /**< Code for a device specific key.*/
	EKeyDevice1B,                        /**< Code for a device specific key.*/
	EKeyDevice1C,                        /**< Code for a device specific key.*/
	EKeyDevice1D,                        /**< Code for a device specific key.*/
	EKeyDevice1E,                        /**< Code for a device specific key.*/
	EKeyDevice1F,                        /**< Code for a device specific key.*/
	EKeyApplication10,                   /**< Code for an Application launcher key.*/
	EKeyApplication11,                   /**< Code for an Application launcher key.*/
	EKeyApplication12,                   /**< Code for an Application launcher key.*/
	EKeyApplication13,                   /**< Code for an Application launcher key.*/
	EKeyApplication14,                   /**< Code for an Application launcher key.*/
	EKeyApplication15,                   /**< Code for an Application launcher key.*/
	EKeyApplication16,                   /**< Code for an Application launcher key.*/
	EKeyApplication17,                   /**< Code for an Application launcher key.*/
	EKeyApplication18,                   /**< Code for an Application launcher key.*/
	EKeyApplication19,                   /**< Code for an Application launcher key.*/
	EKeyApplication1A,                   /**< Code for an Application launcher key.*/
	EKeyApplication1B,                   /**< Code for an Application launcher key.*/
	EKeyApplication1C,                   /**< Code for an Application launcher key.*/
	EKeyApplication1D,                   /**< Code for an Application launcher key.*/
	EKeyApplication1E,                   /**< Code for an Application launcher key.*/
	EKeyApplication1F,                   /**< Code for an Application launcher key.*/
	EKeyDevice20,                        /**< Code for a device specific key.*/
	EKeyDevice21,                        /**< Code for a device specific key.*/
	EKeyDevice22,                        /**< Code for a device specific key.*/
	EKeyDevice23,                        /**< Code for a device specific key.*/
	EKeyDevice24,                        /**< Code for a device specific key.*/
	EKeyDevice25,                        /**< Code for a device specific key.*/
	EKeyDevice26,                        /**< Code for a device specific key.*/
	EKeyDevice27,                        /**< Code for a device specific key.*/
	EKeyApplication20,                   /**< Code for an Application launcher key.*/
	EKeyApplication21,                   /**< Code for an Application launcher key.*/
	EKeyApplication22,                   /**< Code for an Application launcher key.*/
	EKeyApplication23,                   /**< Code for an Application launcher key.*/
	EKeyApplication24,                   /**< Code for an Application launcher key.*/
	EKeyApplication25,                   /**< Code for an Application launcher key.*/
	EKeyApplication26,                   /**< Code for an Application launcher key.*/
	EKeyApplication27                    /**< Code for an Application launcher key.*/
	};

#endif

