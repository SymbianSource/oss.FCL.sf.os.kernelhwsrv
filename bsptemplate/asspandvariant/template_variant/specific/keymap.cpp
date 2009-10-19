// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// template\template_variant\specific\keymap.cpp
// This file is part of the Template Base port
// The keyboard lookup tables giving the function to be carried out
// and the new state of the keyboard
// 
//


#include <k32keys.h>

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))


//
// Scancode conversion tables
// --------------------------
// The scancode conversion is arranged as a tree of tables which are used to
// convert a scancode to a keycode, taking into account the modifier state
// (shift, control, fn)
//
// How the tables work:
// --------------------
// Firstly, there is a distinction between the "scancodes" used in scanning
// the keyboard, and the "scancodes" used in this files.
//
// Typically the keyboard driver already contains a table to convert hardware
// key location codes produced during keyboard scanning into EPOC "scancodes".
// EPOC scancodes are defined for "standard" keys like shift, backspace,
// escape, in the TStdScanCode enum (see E32KEYS.H), and should be ASCII codes
// for normal characters. The keyboard driver should add these EPOC scancodes
// to the event queue, not hardware-dependant key locations.
//
// For now on "scancode" refers to EPOC scancodes:
//
// The keyboard is divided into a number of "blocks" of contiguous scancodes
//
// Blocks map to keycodes in a keycode table, and several blocks can be
// grouped and map to a single keycode table. Blocks map into the keycode
// table in the order they are declared. For example, if two scancode blocks
// with a range of 5 scancodes map to a single 10-entry keycode table, scancodes
// which fall in the first block will map to the first 5 entries in the keycode
// table, scancodes falling in the second block map the the next 5 entries in
// the keycode table.
//
// In theory it is possible to have multiple [keycode,scancode blocks] groups
// but there is little point doing this - grouping all the scancode blocks
// with a single keycode table holding all possible keycode values is usually
// sufficient (and simpler). However, there are some special cases where this
// is useful - the most obvious example is handling of shift and caps lock.
// The shift key implies everything that the caps-lock key does (upper case
// letters) plus some shifted characters for other keys. This is done by 
// defining two tables - the first handles only caps-lock (upper case), the 
// second handles all other keys that are affected only by shift. If caps-
// lock is active, only the caps-lock table is used. If shift is pressed both
// the caps-lock and shift tables are scanned for the conversion. This allows
// a base table to be extended with "extras", much like deriving a class from
// base class and extending it.
//
//
// There is one or more [keycode table, scancode blocks] group for each
// modifier state - e.g. a lower-case table, upper-case, ctrl, ctrl-shift.
// This is the root of the table.
//
// When converting a scancode the key translator first obtains the correct
// conversion tables for the modifier state. It then traverses all the scancode
// blocks looking for one which contains the scancode being converted. Once
// a matching scancode range is located, the key translator maps this into
// its position in the associated keycode table to obtain the converted keycode.
//
// The key tables below appear more complicated than they really are because of
// the intermediate structures that hold pointers to other structures. The
// important structures are:
//		SScanCodeBlock - contains a "start" and "end" for a scancode range
//		SConvSubTable - groups a number of scanode blocks with a keycode table
//		SConvTableNode - points to SConvSubTables for each modifier state
//		SConvTable - the root of the translation table - points to 1..n SConvTableNode
//
// The keycode tables are just an array of TUint16.
//


//
// TO DO: (optional)
//
// Keys which are not affected by modifier state
//

//
// This is a simple example of scancode to keycode mapping. The first block
// in scanCodeBlock_unmodifiable is a range of several scancodes, so maps to
// several entries in the keycode table convKeyCodes_unmodifiable.
//		EStdKeyLeftShift -> maps to -> EKeyLeftShift
//		EStdKeyRightShift -> maps to -> EKeyRightShift
//		...
//		EStdKeyScrollLock -> maps to -> EKeyScrollLock
//
LOCAL_D const SScanCodeBlock scanCodeBlock_unmodifiable[]=
	{
	{EStdKeyLeftShift, EStdKeyScrollLock},	// range 1: left shift to scroll lock
	};

LOCAL_D const TUint16 convKeyCodes_unmodifiable[]=
	{
	EKeyLeftShift,
	EKeyRightShift,
	EKeyLeftAlt,
	EKeyRightAlt,
	EKeyLeftCtrl,
	EKeyRightCtrl,
	EKeyLeftFunc,
	EKeyRightFunc,
	EKeyCapsLock,
	EKeyNumLock,
	EKeyScrollLock
	};



//
// TO DO: (optional)
//
// Base conversion table
// this table traps all of the keyboard's scanCodes except those in
// convKeyCodes_unmodifiable. It appears last in the top-level table and
// is used to convert any scancode that is not converted by any of the
// other tables
//
LOCAL_D const SScanCodeBlock scanCodeBlock_base[]=
	{
	{EStdKeyNull, EStdKeyDownArrow},		// scancode range 1
	{'0', '9'},								// scancode range 2
	{'A', 'Z'},								// scancode range 3
	{EStdKeyF1, EStdKeyDictaphoneRecord},	// scancode range 4
	};

LOCAL_D const TUint16 convKeyCodes_base[]=
	{
	EKeyNull,				// scancode range 1 mapping starts here
	EKeyBackspace,
	EKeyTab,
	EKeyEnter,
	EKeyEscape,
	' ',
	EKeyPrintScreen,
	EKeyPause,
	EKeyHome,
	EKeyEnd,
	EKeyPageUp,
	EKeyPageDown,
	EKeyInsert,
	EKeyDelete,
	EKeyLeftArrow,
	EKeyRightArrow,
	EKeyUpArrow,
	EKeyDownArrow,
	'0',					// scancode range 2 mapping starts here
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
	'a',					// scancode range 3 mapping starts here
	'b',
	'c',
	'd',
	'e',
	'f',
	'g',
	'h',
	'i',
	'j',
	'k',
	'l',
	'm',
	'n',
	'o',
	'p',
	'q',
	'r',
	's',
	't',
	'u',
	'v',
	'w',
	'x',
	'y',
	'z',
	EKeyF1,						// scancode range 4 mapping starts here
	EKeyF2,
	EKeyF3,
	EKeyF4,
	EKeyF5,
	EKeyF6,
	EKeyF7,
	EKeyF8,
	EKeyF9,
	EKeyF10,
	EKeyF11,
	EKeyF12,
	EKeyF13,
	EKeyF14,
	EKeyF15,
	EKeyF16,
	EKeyF17,
	EKeyF18,
	EKeyF19,
	EKeyF20,
	EKeyF21,
	EKeyF22,
	EKeyF23,
	EKeyF24,
	'`',
	',',
	'.',
	'/',
	'\\',
	';',
	'\'',
	'#',
	'[',
	']',
	'-',
	'=',
	'/',
	'*',
	'-',
	'+',
	EKeyEnter,
	EKeyEnd,
	EKeyDownArrow,
	EKeyPageDown,
	EKeyLeftArrow,
	EKeyNull, // numeric keypad '5'
	EKeyRightArrow,
	EKeyHome,
	EKeyUpArrow,
	EKeyPageUp,
	EKeyInsert,
	EKeyDelete,
	EKeyMenu,
	EKeyBacklightOn,
	EKeyBacklightOff,
	EKeyBacklightToggle,
	EKeyIncContrast,
	EKeyDecContrast,
	EKeySliderDown,
	EKeySliderUp,
	EKeyDictaphonePlay,
	EKeyDictaphoneStop,
	EKeyDictaphoneRecord
	};


//
// TO DO: (optional)
//	
// caps-lock: this table traps those scanCodes which are affected by caps-lock
//
LOCAL_D const SScanCodeBlock scanCodeBlock_capsLock[]=
	{
	{'A', 'Z'}			// only alpha keys are affected by caps-lock
	};

LOCAL_D const TUint16 convKeyCodes_capsLock[]=
	{
	'A',
	'B',
	'C',
	'D',
	'E',
	'F',
	'G',
	'H',
	'I',
	'J',
	'K',
	'L',
	'M',
	'N',
	'O',
	'P',
	'Q',
	'R',
	'S',
	'T',
	'U',
	'V',
	'W',
	'X',
	'Y',
	'Z'
	};

//
// TO DO: (optional)
//
// shift: this table traps those scanCodes which are affected
// by normal shift key EXCEPT for those scanCodes affected by caps-lock
//

LOCAL_D const SScanCodeBlock scanCodeBlock_shift[]=
	{
	{'0', '9'},
	{EStdKeyXXX, EStdKeyEquals},
	};

LOCAL_D const TUint16 convKeyCodes_shift[]=
	{
	')',
	'!',
	'@',/*'"',*/
	'#',			/*ELatin1Pound,*/
	'$',
	'%',
	'^',
	'&',
	'*',
	'(',
	'~',   /*ELatin1LogicNot,*/
	'<',
	'>',
	'?',
	'|',
	':',
	'"',
	'|', /*'~',*/
	'{',
	'}',
	'_',
	'+'
	};

//
// TO DO: (optional)
//
// func: this table traps those scanCodes which are affected
// by the func key but not shift
//
LOCAL_D const SScanCodeBlock scanCodeBlock_func[]=
	{
    {EStdKeyEscape, EStdKeyEscape},
    {'M', 'M'},
    {EStdKeyComma, EStdKeyComma},
    {EStdKeyLeftArrow, EStdKeyDownArrow},
	};

LOCAL_D const TUint16 convKeyCodes_func[]=
	{
    EKeyOff,
    EKeyDecContrast,
    EKeyIncContrast,
    EKeyHome,
    EKeyEnd,
    EKeyPageUp,
    EKeyPageDown,
	};

//
// TO DO: (optional)
//
// func: this table traps those scanCodes which are affected
// by func and shift - this is for func pressed, shift not pressed
//
//LOCAL_D const SScanCodeBlock scanCodeBlock_funcUnshifted[]=
//	{
//	{'E', 'E'},
//	};

//LOCAL_D const TUint16 convKeyCodes_funcUnshifted[]=
//	{
//	ELatin1LcEacute,
//	};

//
// TO DO: (optional)
//
// func: this table traps those scanCodes which are affected
// by func and shift - this is for func and shift both pressed
//
//LOCAL_D const SScanCodeBlock scanCodeBlock_funcShifted[]=
//	{
//	{'E', 'E'},
//	};

//LOCAL_D const TUint16 convKeyCodes_funcShifted[]=
//	{
//	ELatin1UcEacute,
//	};

//
// TO DO: (optional)
//
// ctrl: this table traps those scanCodes which are affected by ctrl
//
LOCAL_D const SScanCodeBlock scanCodeBlock_ctrl[]=
	{
	//
	// NOTE: The space key gets handled elsewhere, otherwise it gets
	// thrown away as a NULL key
	//	{EStdKeySpace, EStdKeySpace},

	{'A', 'Z'},
    {EStdKeyComma, EStdKeyComma},
	};

LOCAL_D const TUint16 convKeyCodes_ctrl[]=
	{
//	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	23,
	24,
	25,
	26,
	',',
	};



//
// TO DO: (optional)
//
// Each set of scancode+keycode tables must be grouped into a SConvSubTable.
// The lines below define a number of SConvSubTables for each of the groups
// above.
//
LOCAL_D const SConvSubTable
	convSubTable_unmodifiable=							// table for unmodifiable keys
		{
		&convKeyCodes_unmodifiable[0],					// the keycode table
			{
			ARRAY_LENGTH(scanCodeBlock_unmodifiable),	// number of scancode blocks
			&scanCodeBlock_unmodifiable[0]				// pointer to scancode blocks
			}
		},
	convSubTable_base=									// table for base keys
		{
		&convKeyCodes_base[0],							// keycode table
			{
			ARRAY_LENGTH(scanCodeBlock_base),			// number of scancode blocks
			&scanCodeBlock_base[0]						// pointer to scancode blocks
			}
		},
	convSubTable_capsLock=
		{
		&convKeyCodes_capsLock[0],
			{
			ARRAY_LENGTH(scanCodeBlock_capsLock),
			&scanCodeBlock_capsLock[0]
			}
		},
	convSubTable_shift=
		{
		&convKeyCodes_shift[0],
			{
			ARRAY_LENGTH(scanCodeBlock_shift),
			&scanCodeBlock_shift[0]
			}
		},
	convSubTable_func=
		{
		&convKeyCodes_func[0],
			{
			ARRAY_LENGTH(scanCodeBlock_func),
			&scanCodeBlock_func[0]
			}
		},
//	convSubTable_funcUnshifted=
//		{
//		&convKeyCodes_funcUnshifted[0],
//			{
//			ARRAY_LENGTH(scanCodeBlock_funcUnshifted),
//			&scanCodeBlock_funcUnshifted[0]
//			}
//		},
//	convSubTable_funcShifted=
//		{
//		&convKeyCodes_funcShifted[0],
//			{
//			ARRAY_LENGTH(scanCodeBlock_funcShifted),
//			&scanCodeBlock_funcShifted[0]
//			}
//		},
	convSubTable_ctrl=
		{
		&convKeyCodes_ctrl[0],
			{
			ARRAY_LENGTH(scanCodeBlock_ctrl),
			&scanCodeBlock_ctrl[0]
			}
		};

//
// TO DO: (optional)
//
// We need to declare arrays of SConvSubTable for each modifier state we
// are going to handle. As mentioned above, it is possible to have several
// [keycode table, scancode blocks] groups scanned for each keyboard state.
//
// Some modifier states use more than one conversion group. The simple example
// is handling of caps-lock and shift. 
//
// Caps-lock means all letters are upper-case
// shift means all letters are upper case AND some other keys return control characters
//
// Obviously the shift key means everything cpas-lock means PLUS a bit more. So
// we define two tables, the caps-lock table defines only the uppercase conversion,
// and the shift table defines all OTHER shifted keys not already handled by
// caps-lock. The caps-lock modifier state then only scans the caps-lock table, and
// the shift state scans both tables.
//
LOCAL_D const SConvSubTable
	* const convSubTableArray_unmodifiable[]={&convSubTable_unmodifiable},
	* const convSubTableArray_base[]={&convSubTable_base},

	//
	// The caps-lock state scans only the caps-lock table, to handle
	// conversion to upper case
	//
	* const convSubTableArray_capsLock[]={&convSubTable_capsLock},
	//
	// The shift table scans the caps-lock table to handle upper case, 
	// and also the shift table which handles some keys that are not affected
	// by caps lock (such as 0-9).
	//
	* const convSubTableArray_shift[]={&convSubTable_capsLock, &convSubTable_shift},
	//
	// Pressing shift with caps-lock active reverts to lower-case letters,
	// but other keys remain shifted. This time we only scan the shift table
	// so only the non-alpha keys will be shifted
	//
	* const convSubTableArray_capsLockShift[]={&convSubTable_shift},

	//
	// Like the shift/caps-lock situation, the function key has two states,
	// shifted and unshifted. Also, some keys may be independant of whether
	// the shift key is pressed. So there are three tables defined. One declares
	// all keys that are independant of shift state, the other two tables handle
	// shifted and unshifted func.
	//
	// Unshifted func uses the independant set + funcUnshifted
	//
	//	* const convSubTableArray_func[]={&convSubTable_func, &convSubTable_funcUnshifted},
	* const convSubTableArray_func[]={&convSubTable_func},
	//
	// Shifted func uses the independant set + funcShifted
	//
	//	* const convSubTableArray_funcShift[]={&convSubTable_func,&convSubTable_funcShifted},
	//
	// This keyboard table makes control independant of func and shift
	//
	* const convSubTableArray_ctrl[]={&convSubTable_ctrl};

//
// TO DO: (optional)
//
// This is the top of the scancode conversion tree. It is a set of pointers
// to the SConvSubTable arrays declared above.
//
// The order of these nodes is VITAL, as the scanCode/modifiers are
// searched for a match in this order
//
// The modifier state is matched by using a mask and a compare value. This is
// used as follows:
//
//	match is true if ( (modifierState & mask) == compareValue
//
// For example, if the mask is (EModifierFunc|EModifierShift) and the
// compare value is EModifierFunc, this will match ANY combination of
// modifiers that has func pressed and shift not pressed
//
LOCAL_D const SConvTableNode convTableNodes[]=
	{
		{
			{
			0,		// modifier mask = no modifiers
			0		// modifier compare = no modifiers
			},
		ARRAY_LENGTH(convSubTableArray_unmodifiable),	// number of SConvSubTables
		&convSubTableArray_unmodifiable[0]				// pointer to SConvSubTable array
		},
		{
			{
			EModifierCtrl,	// modifier mask = check for ctrl
			EModifierCtrl	// modifier compare = anything with ctrl pressed
			},
		ARRAY_LENGTH(convSubTableArray_ctrl),
		&convSubTableArray_ctrl[0]
		},
		{
			{
			//
			// Check for Func pressed
			//
			EModifierFunc,
			EModifierFunc
			},
		ARRAY_LENGTH(convSubTableArray_func),
		&convSubTableArray_func[0]
		},
		{
			{
			//
			// Check for caps-lock pressed, shift not pressed
			//
			EModifierCapsLock|EModifierShift,
			EModifierCapsLock
			},
		ARRAY_LENGTH(convSubTableArray_capsLock),
		&convSubTableArray_capsLock[0]
		},
		{
			{
			//
			// Check for caps-lock not pressed, shift pressed
			//
			EModifierShift|EModifierCapsLock,
			EModifierShift
			},
		ARRAY_LENGTH(convSubTableArray_shift),
		&convSubTableArray_shift[0]
		},
		{
			{
			//
			// Check for caps-lock pressed, shift pressed
			//
			EModifierCapsLock|EModifierShift,
			EModifierCapsLock|EModifierShift
			},
		ARRAY_LENGTH(convSubTableArray_capsLockShift),
		&convSubTableArray_capsLockShift[0]
		},
		{
		//
		// This is the base table. It must appear last so that it can
		// provide a default conversion for any scancodes that are not
		// handled by any of the tables above
		//
			{
			0,
			0
			},
		ARRAY_LENGTH(convSubTableArray_base),
		&convSubTableArray_base[0]
		}
	};

//
// The top-level exported data structure of all the conversion tables
// This just points to the SConvTableNodes above
//
LOCAL_D const SConvTable ConvTable=
	{
	ARRAY_LENGTH(convTableNodes),
	&convTableNodes[0]
	};

// The list of scan-codes on the numeric keypad
LOCAL_D const SScanCodeBlock keypadScanCodeBlockArray[]=
	{
	{EStdKeyNumLock, EStdKeyNumLock},
	{EStdKeyNkpForwardSlash, EStdKeyNkpFullStop}
	};

LOCAL_D const SScanCodeBlockList ConvTableKeypadScanCodes=
	{
	ARRAY_LENGTH(keypadScanCodeBlockArray),
	&keypadScanCodeBlockArray[0]
	};

//
// TO DO: (optional)
//
// List of keycodes that do not autorepeat
//
// These are usually control keys like shift, ctrl, escape
//
LOCAL_D const TUint16 nonAutorepKeyCodeArray[]=
	{
	EKeyEscape,
	EKeyPrintScreen,
	EKeyPause,
	EKeyInsert,
	EKeyLeftShift,
	EKeyRightShift,
	EKeyLeftAlt,
	EKeyRightAlt,
	EKeyLeftCtrl,
	EKeyRightCtrl,
	EKeyLeftFunc,
	EKeyRightFunc,
	EKeyCapsLock,
	EKeyNumLock,
	EKeyScrollLock,
	EKeyMenu,
	EKeyDictaphonePlay,
	EKeyDictaphoneStop,
	EKeyDictaphoneRecord
	};

//
// TO DO: (optional)
//
// Declare blocks of non-autorepeating keycodes
//
LOCAL_D const SKeyCodeList ConvTableNonAutorepKeyCodes=
	{
	ARRAY_LENGTH(nonAutorepKeyCodeArray),	// number of keycode arrays
	&nonAutorepKeyCodeArray[0]				// pointer to arrays
	};






/////////////////////////////////////////////////////////////////////
// Keyboard state tables
//

// What these tables do
// --------------------
//
// These tables control the way "special" keystrokes modify the behaviour
// of the keyboard. There are two major uses for this:
//
//	- handling modifier keys e.g. caps-lock, shift, alt, fn and defining
//		what modifier flags are affected by these keypresses
//
//	- switching the keyboard into special states (see below)
//
// Keyboard states
// ---------------
// 
// Keyboard states are used to switch the keyboard into a special mode where it
// can be used to enter unusual characters. There are two uses for this:
//
// - entering numeric codes, by pressing ctrl and typing the decimal code
// - entering accented characters by pressing a key combination which
//		enters "accented mode" then pressing a key. There can be multiple
//		accented modes.
//
// You can see an example of accented modes on a Psion Series 5 by
// pressing Fn+Z, followed by A - this will produce an a with an umlaut (ä)
//
// These tables are also used to select simpler states such as caps-lock
// and num-lock.
//
//
// The main data structure is a SFuncTableEntry. Each of these contains
// three fields:
//
// 1. modifier match - this works the same way as the scancode conversion
//     tables above, there is a mask and a compare value
//
// 2. a keycode patters - this is used to match with the keycode or keycodes
//     that the state should respond to. This is a TKeyCodePattern structure
//     which defines what sort of match should be performed.
//
// 3. a function and state change structure, SFuncAndState. This defines the
//     state to change to, the function to perform, and a parameter for the
//     function if required.
//
// TKeyCodePattern structures have two fields. The first is a keycode value
// and is only used for some match types. The second field select the type
// of match to perform:
//
//	EAnyKey - match any key
//	EAnyAlphaNumeric - match any alpha or numeric key
//	EAnyAlpha - match any alpha key
//	EAnyAlphaLowerCase - match any lower-case key
//	EAnyAlphaUpperCase - match any upper-case key
//	EAnyDecimalDigit - match any decimal digit
//	EAnyModifierKey - match any modifier key (e.g. alt, fn, ctrl)
//	EMatchKey - match if equal to keycode value in first field
//	EMatchLeftOrRight - match if equal to keycode value or (keycode value + 1)
//	EMatchKeyCaseInsens - like EMatchKey but perform case-insensitive comparison
//
//

// the "array" parameter must be a true array and not a pointer
#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))

#define TABLE_ENTRY_ANOTHER_CTRL_DIGIT					\
	{  													\
		{												\
		EModifierKeyUp|EModifierFunc,					\
		0												\
		},												\
		{												\
		EKeyNull,										\
		EAnyDigitGivenRadix								\
		},												\
		{												\
		EStateCtrlDigits,								\
		EAddOnCtrlDigit,								\
		0												\
		}												\
	}

//
// TO DO: (optional)
//
// This table is searched for a match if a match has not been
// found in the current state's table
//

LOCAL_D const SFuncTableEntry defaultTable[]=
	{
		{ 
		//
		// prevent key up events generating keycodes
		//
			{
			EModifierKeyUp,		// mask = key up
			EModifierKeyUp		// match = key up - i.e. accept any key up event
			},
			{
			EKeyNull,			// dummy value, not used
			EAnyKey				// accept any key
			},
			{
			EStateUnchanged,	// state will not change
			EDoNothing,			// no action to perform
			0
			}
		},
		{ 
		//
		// prevent any modifier key (e.g. shift, ctrl) from changing state
		//
			{
			0,					// match any modifier state
			0
			},
			{
			EKeyNull,			// dummy value
			EAnyModifierKey		// match any modifier key
			},
			{
			EStateUnchanged,	// don't change state
			EDoNothing,			// nothing to do
			0
			}
		},
		{ 
		//
		// filter out any unprocessed codes
		//
			{
			0,					// match any modifier state
			0
			},
			{
			EKeyNull,			// dummy value
			EAnyKey				// match any key
			},
			{
			EStateNormal,		// switch back to normal keyboard state
			EDoNothing,			// nothing to do
			0
			}
		}
	};

//
// TO DO: (optional)
//
// This table controls which keys change which modifiers;
// NOTE: the state field in this table is ignored
//

LOCAL_D const SFuncTableEntry modifierTable[]=
	{
		{
			{
			EModifierKeyUp,		// check key-up modifier flag
			0					// make sure it's zero (i.e. ignore key-up events)
			},
			{
			//
			// Here we want to match only the caps-lock key. We specify the
			// keycode we are looking for in the first field, and EMatchKey
			// in the second field
			//
			EKeyCapsLock,		// we want to respond to caps-lock key
			EMatchKey			// match caps-lock only
			},
			{
			EStateUnchanged,	// ignored
			EToggleModifier,	// function = toggle modifier state
			EModifierCapsLock	// this is the modifier to toggle
			}
		},
		{
			{
			EModifierKeyUp,
			0
			},
			{
			EKeyNumLock,		// this one matched num-lock
			EMatchKey			// match only num-lock
			},
			{
			EStateUnchanged,	// ignored
			EToggleModifier,	// function = toggle modifier state
			EModifierNumLock	// this is the modifier to toggle
			}
		},
		{
			{
			EModifierKeyUp,
			0
			},
			{
			EKeyScrollLock,		// match scroll-lock key
			EMatchKey
			},
			{
			EStateUnchanged,
			EToggleModifier,	// function = toggle modifier
			EModifierScrollLock	// modifier to toggle
			}
		},
		{
			{
			EModifierKeyUp,
			0
			},
			{
			EKeyLeftAlt,		// match left alt key
			EMatchKey
			},
			{
			EStateUnchanged,	// ignored
			ETurnOnModifier,	// function = turn on a modifier
			EModifierAlt|EModifierLeftAlt	// alt turns on this modifier combination
			}
		},
		{
			{
			EModifierKeyUp,		// goes with previous table, this handles the alt
			EModifierKeyUp		// key being released
			},
			{
			EKeyLeftAlt,		// match left alt key again
			EMatchKey
			},
			{
			EStateUnchanged,
			ETurnOffModifier,	// function = turn off the modifier
			EModifierLeftAlt	// modifier to turn off
			}
		},
		{
			{
			EModifierKeyUp,		// key down event (key-up flag == 0)
			0
			},
			{
			EKeyLeftFunc,		// match left fn key
			EMatchKey
			},
			{
			EStateUnchanged,	// ignored
			ETurnOnModifier,	// function = turn on modifier
			EModifierFunc|EModifierLeftFunc	// modifier combination to turn on
			}
		},
		{
			{
			EModifierKeyUp,		// goes with above table, this matched the
			EModifierKeyUp		// left-fn key up event
			},
			{
			EKeyLeftFunc,		// match left fn key
			EMatchKey
			},
			{
			EStateUnchanged,	// ignored
			ETurnOffModifier,	// function = turn off modifier
			EModifierLeftFunc	// modifier to turn off
			}
		},
		{
			{
			EModifierKeyUp,		// key down event (key-up flag == 0)
			0
			},
			{
			EKeyLeftShift,		// match left shift key
			EMatchKey
			},
			{
			EStateUnchanged,	// ignored
			ETurnOnModifier,	// function = turn on modifier
			EModifierShift|EModifierLeftShift	// modifier combination to turn on
			}
		},
		{
			{
			EModifierKeyUp,		// goes with above table, matches left shift
			EModifierKeyUp		// key up event
			},
			{
			EKeyLeftShift,		// match left shift key
			EMatchKey
			},
			{
			EStateUnchanged,	// ignored
			ETurnOffModifier,	// turn off modifier
			EModifierLeftShift	// modifier to turn off
			}
		},
		{
			{
			EModifierKeyUp,		// key down event (key-up flag == 0)
			0
			},
			{
			EKeyLeftCtrl,		// match left ctrl key
			EMatchKey
			},
			{
			EStateUnchanged,	// ignored
			ETurnOnModifier,	// function = turn on modifier
			EModifierCtrl|EModifierLeftCtrl	// modifier combination to turn on
			}
		},
		{
			{
			EModifierKeyUp,		// goes with above table, matches left ctrl
			EModifierKeyUp		// key up event
			},
			{
			EKeyLeftCtrl,		// match left ctrl key
			EMatchKey
			},
			{
			EStateUnchanged,	// ignored
			ETurnOffModifier,	// function = turn off modifier
			EModifierLeftCtrl	// modifier to turn off
			}
		},
		{
			{
			EModifierKeyUp,		// key down event (key-up flag == 0)
			0
			},
			{
			EKeyRightAlt,		// match right alt key
			EMatchKey
			},
			{
			EStateUnchanged,	// ignored
			ETurnOnModifier,	// function = turn on modifier
			EModifierAlt|EModifierRightAlt	// modifier combination to turn on
			}
		},
		{
			{
			EModifierKeyUp,		// goes with above table, matches right alt
			EModifierKeyUp		// key up event
			},
			{
			EKeyRightAlt,		// match right alt key
			EMatchKey
			},
			{
			EStateUnchanged,	// ignored
			ETurnOffModifier,	// function = turn off modifier
			EModifierRightAlt	// modifier to turn off
			}
		},
		{
			{
			EModifierKeyUp,		// key down event (key-up flag == 0)
			0
			},
			{
			EKeyRightFunc,		// match right fn key
			EMatchKey
			},
			{
			EStateUnchanged,	// ignored
			ETurnOnModifier,	// function = turn on modifier
			EModifierFunc|EModifierRightFunc	// modifier combination to turn on
			}
		},
		{
			{
			EModifierKeyUp,		// goes with above table, matches right fn
			EModifierKeyUp		// key up event
			},
			{
			EKeyRightFunc,		// match right fn key
			EMatchKey
			},
			{
			EStateUnchanged,	// ignored
			ETurnOffModifier,	// function = turn off modifier
			EModifierRightFunc	// modifier to turn off
			}
		},
		{
			{
			EModifierKeyUp,		// key down event (key-up flag == 0)
			0
			},
			{
			EKeyRightShift,		// match right shift key
			EMatchKey
			},
			{
			EStateUnchanged,	// ignored
			ETurnOnModifier,	// function = turn on modifier
			EModifierShift|EModifierRightShift	// modifier combinatoin to turn on
			}
		},
		{
			{
			EModifierKeyUp,		// goes with above table, handles right shift
			EModifierKeyUp		// key up event
			},
			{
			EKeyRightShift,		// match right shift key
			EMatchKey
			},
			{
			EStateUnchanged,	// ignored
			ETurnOffModifier,	// function = turn off modifier
			EModifierRightShift	// modifier to turn off
			}
		},
		{
			{
			EModifierKeyUp,		// key down event (key-up flag == 0)
			0
			},
			{
			EKeyRightCtrl,		// match right ctrl key
			EMatchKey
			},
			{
			EStateUnchanged,	// ignored
			ETurnOnModifier,	// function = turn on modifier
			EModifierCtrl|EModifierRightCtrl	// modifier combination to turn on
			}
		},
		{
			{
			EModifierKeyUp,		// goes with above table, matched right ctrl
			EModifierKeyUp		// key up event
			},
			{
			EKeyRightCtrl,		// match right ctrl key
			EMatchKey
			},
			{
			EStateUnchanged,	// ignored
			ETurnOffModifier,	// function = turn off modifier
			EModifierRightCtrl	// modifier to turn off
			}
		}
	};


//
// TO DO: (optional)
//
// Tables corresponding to keyboard states.
//
// There are 13 keyboard states. States 0 to 9 can be used to create alternative
// keyboard layouts for entering accented or unusual characters. Switching into
// these states is done by a keypress. Implementation of the states is optional
// depending on how many special state you want - you may implement 10 states,
// you might decide not to implement any.
//
// State 10 is the normal state. The table for state 10 defines which keypresses
// change to other states.
//
// States 11 and 12 are used when entering the numeric code of a character. State
// 11 is for entering a specific number of digits. State 12 is for accepting
// digits until Ctrl is released.
//	
//
// As before, each SFuncTableEntry entry defines:
//	- modifier conditions that must be matched
//	- a keycode match pattern (typically an exact key match)
//	- the function to perform and new state
//
// Switching into states 0..9,11,12 is done by entries in table10
//

//LOCAL_D const SFuncTableEntry table0[]=
//	{
//	TABLE_ENTRY_ANOTHER_CTRL_DIGIT
//	};

LOCAL_D const SFuncTableEntry table1[]=
	{
	//
	// Table for special keyboard state 1
	// This state is entered by pressing Fn+q (see table10)
	//
	// The table makes certain keys return accented characters
	//
		{
			{
			//
			// Function must be release, and this must be a key down event
			//
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			//
			// match an 'e' keypress, convert to an ae ligature (æ)
			//
			'e',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,			// switch back to state normal (table10)
			EPassSpecialKeyThru,	// turn keypress into a special character
			ELatin1LcAe				// this is the character to pass on
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'c',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcCcedilla
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			's',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1EsTset
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'o',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcOslash
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'd',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcThorn
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			't',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcSoftTh
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'l',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LeftChevron
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'r',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1RightChevron
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'x',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1InvExclam
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'q',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1InvQuest
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'a',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcAo
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'p',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1Pound
			}
		},
	TABLE_ENTRY_ANOTHER_CTRL_DIGIT
	};

LOCAL_D const SFuncTableEntry table2[]=
	{
	//
	// Table for special keyboard state 2
	// This state is entered by pressing Fn+z (see table10)
	//
	// The table makes certain keys return accented characters
	// See table1 for an explanation of the contents
	//
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'a',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcAumlaut
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'e',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcEumlaut
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'i',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcIumlaut
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'o',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcOumlaut
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'u',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcUumlaut
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'y',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcYumlaut
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			' ',
			EMatchKey
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1SpaceUmlaut
			}
		},
	TABLE_ENTRY_ANOTHER_CTRL_DIGIT
	};

LOCAL_D const SFuncTableEntry table3[]=
	{
	//
	// Table for special keyboard state 3
	// This state is entered by pressing Fn+x (see table10)
	//
	// The table makes certain keys return accented characters
	//
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'a',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcAgrave
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'e',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcEgrave
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'i',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcIgrave
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'o',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcOgrave
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'u',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcUgrave
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			' ',
			EMatchKey
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1SpaceGrave
			}
		},
	TABLE_ENTRY_ANOTHER_CTRL_DIGIT
	};

LOCAL_D const SFuncTableEntry table4[]=
	{
	//
	// Table for special keyboard state 4
	// This state is entered by pressing Fn+c (see table10)
	//
	// The table makes certain keys return accented characters
	//
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'a',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcAacute
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'e',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcEacute
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'i',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcIacute
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'o',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcOacute
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'u',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcUacute
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'y',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcYacute
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			' ',
			EMatchKey
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcSpaceAcute
			}
		},
	TABLE_ENTRY_ANOTHER_CTRL_DIGIT
	};

LOCAL_D const SFuncTableEntry table5[]=
	{
	//
	// Table for special keyboard state 5
	// This state is entered by pressing Fn+v (see table10)
	//
	// The table makes certain keys return accented characters
	//
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'a',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcAtilde
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'n',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcNtilde
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'o',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcOtilde
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			' ',
			EMatchKey
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcSpaceTilde
			}
		},
	TABLE_ENTRY_ANOTHER_CTRL_DIGIT
	};

LOCAL_D const SFuncTableEntry table6[]=
	{
	//
	// Table for special keyboard state 6
	// This state is entered by pressing Fn+b (see table6)
	//
	// The table makes certain keys return accented characters
	//
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'a',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcAcirc
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'e',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcEcirc
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'i',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcIcirc
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'o',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcOcirc
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			'u',
			EMatchKeyCaseInsens
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcUcirc
			}
		},
		{
			{
			EModifierFunc|EModifierKeyUp,
			0
			},
			{
			' ',
			EMatchKey
			},
			{
			EStateNormal,
			EPassSpecialKeyThru,
			ELatin1LcSpaceCirc
			}
		},
	TABLE_ENTRY_ANOTHER_CTRL_DIGIT
	};

//
// TO DO: (optional)
//
// State 7,8,9 aren't used in this example.
// You can implement them if you want more special states
//

//LOCAL_D const SFuncTableEntry table7[]=
//	{
//	TABLE_ENTRY_ANOTHER_CTRL_DIGIT
//	};

//LOCAL_D const SFuncTableEntry table8[]=
//	{
//	TABLE_ENTRY_ANOTHER_CTRL_DIGIT
//	};

//LOCAL_D const SFuncTableEntry table9[]=
//	{
//	TABLE_ENTRY_ANOTHER_CTRL_DIGIT
//	};


LOCAL_D const SFuncTableEntry table10[]=
	{
	//
	// TO DO: (optional)
	//
	// Table keyboard state 10 - the normal state
	//
	// This table controls which keys switch into the special states
	// 0-9, 11 and 12.
	//

		{ 
		//
		// Make sure key-up events are ignored by handling them first and
		// doing nothing
		//
			{
			EModifierKeyUp,
			EModifierKeyUp
			},
			{
			EKeyNull,
			EAnyKey
			},
			{
			EStateUnchanged,
			EDoNothing,
			0
			}
		},
		{ 
		//
		// Check for ctrl-number presses
		// This will enter state EStateCtrlDigits (state 12) which allows
		// entry of a numeric keycode
		//
			{
			EModifierCtrl|EModifierFunc|EModifierKeyUp,
			EModifierCtrl
			},
			{
			EKeyNull,
			EAnyDecimalDigit
			},
			{
			EStateDerivedFromDigitEntered,
			EAddOnCtrlDigit,
			0
			}
		},
		{
		//
		// Any other key events that have not been trapped are just
		// passed through unchanged
		//
			{
			0,
			0
			},
			{
			EKeyNull,
			EAnyKey
			},
			{
			EStateUnchanged,
			EPassKeyThru,
			0
			}
		}
	};

//LOCAL_D const SFuncTableEntry table11[]=
//	{
//	TABLE_ENTRY_ANOTHER_CTRL_DIGIT
//	};

LOCAL_D const SFuncTableEntry table12[]=
	{
	//
	// Table 12 handles entring digit codes. The keyboard will remain in this
	// state until the Ctrl key is released
	//
		{
			{
			//
			// Look for a key up event
			//
			EModifierKeyUp,
			EModifierKeyUp
			},
			{
			// 
			// Match either left or right Ctrl key (i.e. this matches a Ctrl key release)
			//
			EKeyLeftCtrl,
			EMatchLeftOrRight
			},
			{
			EStateNormal,			// return to normal state (table10)
			EPassCtrlDigitsThru,	// and pass through the numeric code we have accumulated
			0
			}
		},
	TABLE_ENTRY_ANOTHER_CTRL_DIGIT
	};


//
// TO DO: (optional)
//
// Array of state control tables above. If a state is not used set the array 
// size to zero and the pointer to NULL
//
// The tables must be declared here in order from table 0 to table 12
//
LOCAL_D const SFuncTable genFuncTables[]=
	{
		{
		//
		// state 0
		//
		0,			// state 0 not used, size = 0
		NULL		// state 0 not used, pointer = NULL
		},
		{
		//
		// state 1
		//
		ARRAY_LENGTH(table1),		// size of table 1
		&table1[0]					// pointer to table 1
		},
		{
		//
		// state 2
		//
		ARRAY_LENGTH(table2),
		&table2[0]
		},
		{
		//
		// state 3
		//
		ARRAY_LENGTH(table3),
		&table3[0]
		},
		{
		//
		// state 4
		//
		ARRAY_LENGTH(table4),
		&table4[0]
		},
		{
		//
		// state 5
		//
		ARRAY_LENGTH(table5),
		&table5[0]
		},
		{
		//
		// state 6
		//
		ARRAY_LENGTH(table6),
		&table6[0]
		},
		{
		//
		// state 7
		//
		0,
		NULL
		},
		{
		//
		// state 8
		//
		0,
		NULL
		},
		{
		//
		// state 9
		//
		0,
		NULL
		},
		{
		//
		// state 10
		//
		ARRAY_LENGTH(table10),
		&table10[0]
		},
		{
		//
		// state 11
		//
		0,
		NULL
		},
		{
		//
		// state 12
		//
		ARRAY_LENGTH(table12),
		&table12[0]
		}
	};


//
// Root of the state modifier tables
//
LOCAL_D const SFuncTables FuncTables=
	{
		{
		//
		// The default processing table
		//
		ARRAY_LENGTH(defaultTable),
		&defaultTable[0]
		},
		{
		//
		// The modifier control table
		//
		ARRAY_LENGTH(modifierTable),
		&modifierTable[0]
		},
	//
	// The state control tables
	//
	ARRAY_LENGTH(genFuncTables),
	&genFuncTables[0]
	};


//
// The following exported functions give the key translator access to
// the control tables above
//
EXPORT_C void KeyDataSettings(TRadix &aRadix,TCtrlDigitsTermination &aCtrlDigitsTermination,TInt &aDefaultCtrlDigitsMaxCount,
							  TInt &aMaximumCtrlDigitsMaxCount)
	{
	aRadix=EDecimal;
	aCtrlDigitsTermination=ETerminationByCtrlUp;
	aDefaultCtrlDigitsMaxCount=3;
	aMaximumCtrlDigitsMaxCount=10;
	}

EXPORT_C void KeyDataFuncTable(SFuncTables &aFuncTables)
	{
	aFuncTables=FuncTables;
	}

EXPORT_C void KeyDataConvTable(SConvTable &aConvTable, TUint &aConvTableFirstScanCode,TUint &aConvTableLastScanCode,
							 SScanCodeBlockList &aKeypadScanCode,SKeyCodeList &aNonAutorepKeyCodes)
	{
	aConvTable=ConvTable;
	aConvTableFirstScanCode=scanCodeBlock_base[0].firstScanCode;
	aConvTableLastScanCode=scanCodeBlock_base[ARRAY_LENGTH(scanCodeBlock_base)-1].lastScanCode;
	aKeypadScanCode=ConvTableKeypadScanCodes;
	aNonAutorepKeyCodes=ConvTableNonAutorepKeyCodes;
	}
