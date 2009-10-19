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
// e32\drivers\edisp\epoc\generic\wd_vt100.cpp
// Display/Keyboard driver using VT100 terminal
// 
//

#include "ws_std.h"
#include <d32comm.h>
#include <e32svr.h>

/**
 * Define a comms driver and port to use.
 */

//#define __USE_RDEBUG_OUTPUT
#define __USE_SERIAL_INPUT
const TInt KPortNumber=1;

#if defined(__USE_SERIAL_INPUT) || !defined(__USE_RDEBUG_OUTPUT)
#define __COMMS_DRIVER_NEEDED
_LIT(KPddName,"EUART");
_LIT(KLddName,"ECOMM");
#endif

class CSerialKeyboard : public CActive
	{
public:
	static CSerialKeyboard* NewL();
	~CSerialKeyboard();
	void GetKey();
public:
	CSerialKeyboard();
	void ConstructL();
	void RunL();
	void DoCancel();
	TInt KeyCode();
public:
#ifdef __COMMS_DRIVER_NEEDED
	RBusDevComm iDevComm;
	TBuf8<1> iBuf;
#endif
	};

#define SHIFTED(x)   (0x8000|(x))
#define ISSHIFTED(x) (0x8000&(x))
#define FUNCED(x)    (0x4000|(x))
#define ISFUNCED(x)  (0x4000&(x))
#define CTRLED(x)    (0x2000|(x))
#define ISCTRLED(x)  (0x2000&(x))
#define STDKEY(x)    (0x1FFF&(x))

const TUint16 convertCode[] =
	{
/*00 NUL*/  EStdKeyNull,
/*01 SOH*/  CTRLED('A'),	// ^A
/*02 STX*/  CTRLED('B'),	// ^B
/*03 ETX*/  CTRLED('C'),	// ^C
/*04 EOT*/  CTRLED('D'),	// ^D
/*05 ENQ*/  CTRLED('E'),	// ^E
/*06 ACK*/  CTRLED('F'),	// ^F
/*07 BEL*/  CTRLED('G'),	// ^G
/*08 BS */  EStdKeyBackspace,
/*09 TAB*/  CTRLED(FUNCED('5')),
/*0a LF */  CTRLED('J'),	// ^J
/*0b VT */  CTRLED('K'),	// ^K
/*0c FF */  CTRLED('L'),	// ^L
/*0d CR */  EStdKeyEnter,
/*0e SO */  CTRLED('N'),	// ^N
/*0f SI */  CTRLED('O'),	// ^O
/*10 DLE*/  CTRLED('P'),	// ^P
/*11 DC1*/  CTRLED('Q'),	// ^Q
/*12 DC2*/  CTRLED('R'),	// ^R
/*13 DC3*/  CTRLED('S'),	// ^S
/*14 DC4*/  CTRLED('T'),	// ^T
/*15 NAK*/  CTRLED('U'),	// ^U
/*16 SYN*/  CTRLED('V'),	// ^V
/*17 ETB*/  CTRLED('W'),	// ^W
/*18 CAN*/  CTRLED('X'),	// ^X
/*19 EM */  CTRLED('Y'),	// ^Y
/*1a SUB*/  CTRLED('Z'),	// ^Z
/*1b ESC*/  EStdKeyEscape,
/*1c FS */  EStdKeyNull,	// ^backslash -> ignored
/*1d GS */  EStdKeyNull,	// ^] -> ignored
/*1e RS */  EStdKeyNull,	// ^^ -> ignored
/*1f US */  EStdKeyNull,	// ^_ -> ignored
/*20*/  EStdKeySpace,
/*21*/  SHIFTED('1'),       // !
/*22*/  SHIFTED('2'),       // "
/*23*/  EStdKeyHash,        // #
/*24*/  SHIFTED('4'),       // $
/*25*/  SHIFTED('5'),       // %
/*26*/  SHIFTED('7'),       // &
/*27*/  EStdKeySingleQuote,
/*28*/  SHIFTED('9'),       // (
/*29*/  SHIFTED('0'),       // )
/*2a*/  SHIFTED('8'),       // *
/*2b*/  SHIFTED(EStdKeyEquals), // +
/*2c*/  EStdKeyComma,       // ,
/*2d*/  EStdKeyMinus,       // -
/*2e*/  EStdKeyFullStop,    // .
/*2f*/  EStdKeyForwardSlash,// forward slash
/*30*/  '0',
/*31*/  '1',
/*32*/  '2',
/*33*/  '3',
/*34*/  '4',
/*35*/  '5',
/*36*/  '6',
/*37*/  '7',
/*38*/  '8',
/*39*/  '9',
/*3a*/  SHIFTED(EStdKeySemiColon),  // :
/*3b*/  EStdKeySemiColon,           // ;
/*3c*/  SHIFTED(EStdKeyComma),      // <
/*3d*/  EStdKeyEquals,              // =
/*3e*/  SHIFTED(EStdKeyFullStop),   // >
/*3f*/  SHIFTED(EStdKeyForwardSlash), // ?
/*40*/  SHIFTED(EStdKeySingleQuote),  // @
/*41*/  SHIFTED('A'),
/*42*/  SHIFTED('B'),
/*43*/  SHIFTED('C'),
/*44*/  SHIFTED('D'),
/*45*/  SHIFTED('E'),
/*46*/  SHIFTED('F'),
/*47*/  SHIFTED('G'),
/*48*/  SHIFTED('H'),
/*49*/  SHIFTED('I'),
/*4a*/  SHIFTED('J'),
/*4b*/  SHIFTED('K'),
/*4c*/  SHIFTED('L'),
/*4d*/  SHIFTED('M'),
/*4e*/  SHIFTED('N'),
/*4f*/  SHIFTED('O'),
/*50*/  SHIFTED('P'),
/*51*/  SHIFTED('Q'),
/*52*/  SHIFTED('R'),
/*53*/  SHIFTED('S'),
/*54*/  SHIFTED('T'),
/*55*/  SHIFTED('U'),
/*56*/  SHIFTED('V'),
/*57*/  SHIFTED('W'),
/*58*/  SHIFTED('X'),
/*59*/  SHIFTED('Y'),
/*5a*/  SHIFTED('Z'),
/*5b*/  EStdKeySquareBracketLeft,   // [
/*5c*/  EStdKeyBackSlash,           // backslash
/*5d*/  EStdKeySquareBracketRight,  // ]
/*5e*/  SHIFTED('6'),               // ^
/*5f*/  SHIFTED(EStdKeyMinus),      // _
/*60*/  EStdKeyXXX,					// Actually `
/*61*/  'A',
/*62*/  'B',
/*63*/  'C',
/*64*/  'D',
/*65*/  'E',
/*66*/  'F',
/*67*/  'G',
/*68*/  'H',
/*69*/  'I',
/*6a*/  'J',
/*6b*/  'K',
/*6c*/  'L',
/*6d*/  'M',
/*6e*/  'N',
/*6f*/  'O',
/*70*/  'P',
/*71*/  'Q',
/*72*/  'R',
/*73*/  'S',
/*74*/  'T',
/*75*/  'U',
/*76*/  'V',
/*77*/  'W',
/*78*/  'X',
/*79*/  'Y',
/*7a*/  'Z',
/*7b*/  SHIFTED(EStdKeySquareBracketLeft),  // {
/*7c*/  SHIFTED(EStdKeyBackSlash),          // |
/*7d*/  SHIFTED(EStdKeySquareBracketRight),	// }
/*7e*/  SHIFTED(EStdKeyHash),				// ~
/*7f*/  EKeyDelete
	};

CSerialKeyboard* CSerialKeyboard::NewL()
	{
	CSerialKeyboard* self = new(ELeave) CSerialKeyboard;
	self->ConstructL();
	return self;
	}

CSerialKeyboard::CSerialKeyboard()
	:	CActive(0)
	{
	}

void CSerialKeyboard::ConstructL()
	{
#ifdef __COMMS_DRIVER_NEEDED
	// load and connect to serial port
	TInt r=User::LoadPhysicalDevice(KPddName);
	if (r!=KErrNone && r!=KErrAlreadyExists)
		User::Leave(r);
	r=User::LoadLogicalDevice (KLddName);
	if (r!=KErrNone && r!=KErrAlreadyExists)
		User::Leave(r);

	r=iDevComm.Open(KPortNumber);
	User::LeaveIfError(r);

	TCommConfig cfgBuf;
	TCommConfigV01& cfg=cfgBuf();
	iDevComm.Config(cfgBuf);
	cfg.iRate=EBps115200;
	cfg.iDataBits=EData8;
	cfg.iStopBits=EStop1;
	cfg.iParity=EParityNone;
	cfg.iHandshake=0;//KConfigObeyXoff|KConfigSendXoff;
	cfg.iFifo=EFifoEnable;
	cfg.iTerminatorCount=0;
	cfg.iSIREnable=ESIRDisable;
	User::LeaveIfError(iDevComm.SetConfig(cfgBuf));
#endif

	CActiveScheduler::Add(this);
	}

CSerialKeyboard::~CSerialKeyboard()
	{
	Cancel();
	}

void CSerialKeyboard::GetKey()
	{
	__ASSERT_ALWAYS(!IsActive(), User::Panic(_L("CSerialKeyboard"),1));

	// wait for a key
#ifdef __USE_SERIAL_INPUT
	iDevComm.Read(iStatus,iBuf,1);
#endif
	SetActive();
	}

TInt CSerialKeyboard::KeyCode()
	{
	TInt c=-1;
#ifdef __USE_SERIAL_INPUT
	if (iBuf.Length()>0)
		c=iBuf[0];
#endif
	return c;
	}

void CSerialKeyboard::RunL()
	{
	TInt c=KeyCode();
	if (c>=0)
		{
		// convert character to keycode and shift, func, ctrl status
		TUint16 code = convertCode[c];
		TBool isShifted = ISSHIFTED(code);
		TBool isFunced = ISFUNCED(code);
		TBool isCtrled = ISCTRLED(code);
		TUint8 stdKey = STDKEY(code);
		TRawEvent e;

		// post it as a sequence of events
		if (isShifted)
			{
			e.Set(TRawEvent::EKeyDown,EStdKeyRightShift,0);
			UserSvr::AddEvent(e);
			}
		if (isCtrled)
			{
			e.Set(TRawEvent::EKeyDown,EStdKeyLeftCtrl,0);
			UserSvr::AddEvent(e);
			}
		if (isFunced)
			{
			e.Set(TRawEvent::EKeyDown,EStdKeyLeftFunc,0);
			UserSvr::AddEvent(e);
			}

		e.Set(TRawEvent::EKeyDown,stdKey,0);
		UserSvr::AddEvent(e);

		e.Set(TRawEvent::EKeyUp,stdKey,0);
		UserSvr::AddEvent(e);

		if (isFunced)
			{
			e.Set(TRawEvent::EKeyUp,EStdKeyLeftFunc,0);
			UserSvr::AddEvent(e);
			}
		if (isCtrled)
			{
			e.Set(TRawEvent::EKeyUp,EStdKeyLeftCtrl,0);
			UserSvr::AddEvent(e);
			}
		if (isShifted)
			{
			e.Set(TRawEvent::EKeyUp,EStdKeyRightShift,0);
			UserSvr::AddEvent(e);
			}
		}

	// get another key
	GetKey();
	}

void CSerialKeyboard::DoCancel()
	{
#ifdef __USE_SERIAL_INPUT
	iDevComm.ReadCancel();
#endif
	}


class CScreenDriverVT100 : public CScreenDriver
	{
public:
	CScreenDriverVT100();
	virtual void Init(TSize &aScreenSize,TSize &aFontSize);
	virtual void Blit(const TText *aBuffer,TInt aLength,const TPoint &aPosition);
	virtual TBool ScrollUp(const TRect& aRect);
	virtual void Clear(const TRect& aRect);

	virtual void SetPixel(const TPoint& aPoint,TUint8 aColour);
	virtual TInt GetPixel(const TPoint& aPoint);
	virtual void SetWord(const TPoint& aPoint,TInt aWord);
	virtual TInt GetWord(const TPoint& aPoint);
	virtual void SetLine(const TPoint& aPoint,const TPixelLine& aPixelLine);
	virtual void GetLine(const TPoint& aPoint,TPixelLine& aPixelLine);

	virtual TInt SetMode(TVideoMode aMode);

	virtual void SetPaletteEntry(TColorIndex anIndex,TUint8 aRed,TUint8 aGreen,TUint8 aBlue) {}
	virtual void GetPaletteEntry(TColorIndex anIndex,TUint8 &aRed,TUint8 &aGreen,TUint8 &aBlue) {}
	virtual void SetForegroundColor(TColorIndex anIndex) {}
	virtual void SetBackgroundColor(TColorIndex anIndex) {}
	virtual void GetAttributeColors(TColorIndex* anArray) {}

	//
	void Update(const TRect &aRect);
	void Print(const TDesC8& aDes);
	TUint8 *iScreenBuffer;
private:
	CSerialKeyboard *iSerialKeyboard;
	};

const TInt KScreenWidth = 80;
const TInt KScreenHeight = 24;

CScreenDriverVT100::CScreenDriverVT100()
//
// Constructor
//
	{
	}

EXPORT_C CScreenDriver *CScreenDriver::New()
//
// Return the actual screen driver.
//
	{

    CScreenDriverVT100 *pS=new CScreenDriverVT100;
	pS->iScreenBuffer=new TUint8 [KScreenWidth*KScreenHeight];
	return pS;
	}

void CScreenDriverVT100::Init(TSize& aScreenSize, TSize& aFontSize)
	{
	// Report screen information
	aFontSize=TSize(8,10);
	aScreenSize=TSize(KScreenWidth,KScreenHeight);

	// start keyboard
	TRAPD(r,iSerialKeyboard = CSerialKeyboard::NewL());
    // must panic if there are not enough resources to continue
    __ASSERT_ALWAYS(r==KErrNone, User::Panic(_L("CSerialKeyboard"),2));

	iSerialKeyboard->GetKey();
	}

TInt CScreenDriverVT100::SetMode(TVideoMode aMode)
//
// Set the screen mode
//
	{
	return(KErrNotSupported);
	}

void CScreenDriverVT100::Update(const TRect &aRect)
	{
	TBuf8<0x100> buf;
	TInt i;

	for (i=aRect.iTl.iY; i<=aRect.iBr.iY; i++)
		{
		buf.Format(_L8("\x1b[%02d;%02dH"), i+1, aRect.iTl.iX+1);
		TPtrC8 ptr(iScreenBuffer+(aRect.iTl.iX+i*KScreenWidth),aRect.iBr.iX-aRect.iTl.iX);
		buf.Append(ptr);
		Print(buf);
		}
	Print(_L8("\x1b[01;01H"));
	}

void CScreenDriverVT100::Blit(const TText *aBuffer, TInt aLength, const TPoint &aPosition)
//
// Write contiguous block of characters to some segment of a line on the display
//
	{
	TUint8 *ptr=iScreenBuffer+(aPosition.iX+aPosition.iY*KScreenWidth);
	const TText* endBuf = aBuffer + aLength;
	while (aBuffer < endBuf)
		*ptr++ = (TUint8)*aBuffer++;
	Update(TRect(aPosition.iX, aPosition.iY, aPosition.iX+aLength, aPosition.iY));
	}

TBool CScreenDriverVT100::ScrollUp(const TRect& aRect)
//
// Scroll a rectangle of the screen up one line, don't update bottom line
//
	{
	TInt j;
	for (j=aRect.iTl.iY; j<aRect.iBr.iY; j++)
		{
		TUint8 *ptr=iScreenBuffer+(aRect.iTl.iX+j*KScreenWidth);
		Mem::Copy(ptr, ptr+KScreenWidth, aRect.iBr.iX-aRect.iTl.iX);
		}
	if ((aRect.iTl.iX<=1) && (aRect.iBr.iX>=KScreenWidth-2))
		{
		// Optimisation: range of whole lines
		TBuf8<0x100> buf;
		// cursor pos
		buf.Format(_L8("\x1b[%02d;%02dH\xd\xa\xba\x1b[%02dC\xba"), aRect.iBr.iY, 1, aRect.iBr.iX);
		Print(buf);
		// set scroll region
		buf.Format(_L8("\x1b[%02d;%02dr\xd\xa"), aRect.iTl.iY+1, aRect.iBr.iY);
		Print(buf);
		Print(_L8("\x1b[01;01H"));
		}
	else
		Update(aRect);

	return(ETrue);
	}

void CScreenDriverVT100::Clear(const TRect& aRect)
//
// Clear a rectangle of the screen
//
	{
	TInt j;
	TBuf8<0x100> buf;
	for (j=aRect.iTl.iY; j<=aRect.iBr.iY; j++)
		{
		TUint8 *ptr=iScreenBuffer+(aRect.iTl.iX+j*KScreenWidth);
		Mem::FillZ(ptr, aRect.iBr.iX-aRect.iTl.iX);
		}
	if ((aRect.iTl.iY == aRect.iBr.iY) && (aRect.iTl.iX==0) && (aRect.iBr.iX==KScreenWidth-1))
		{
		// Optimisation: one whole line
		buf.Format(_L8("\x1b[%02d;%02dH"), aRect.iTl.iY, 1);
		Print(buf);
		// Erase line
		buf.Format(_L8("\x1b[2K"));
		Print(buf);
		Print(_L8("\x1b[01;01H"));
		}
	else if ((aRect.iTl.iY == 0) && (aRect.iBr.iY == KScreenHeight-1) &&
						(aRect.iTl.iX == 0) && (aRect.iBr.iX == KScreenWidth-1))
		{
		// Optimisation: whole screen
		buf.Format(_L8("\x1b[2J"));
		Print(buf);
		Print(_L8("\x1b[01;01H"));
		}
	else
		Update(aRect);
	}

void CScreenDriverVT100::SetPixel(const TPoint& /*aPoint*/,TUint8 /*aColour*/)
	{
	}

TInt CScreenDriverVT100::GetPixel(const TPoint& /*aPoint*/)
	{
	return 0;
	}

void CScreenDriverVT100::SetWord(const TPoint& /*aPoint*/,TInt /*aWord*/)
	{
	}

TInt CScreenDriverVT100::GetWord(const TPoint& /*aPoint*/)
	{
	return 0;
	}

void CScreenDriverVT100::SetLine(const TPoint& /*aPoint*/,const TPixelLine& /*aPixelLine*/)
	{
	}

void CScreenDriverVT100::GetLine(const TPoint& /*aPoint*/,TPixelLine& /*aPixelLine*/)
	{
	}

void CScreenDriverVT100::Print(const TDesC8& aDes)
	{
#ifdef __USE_RDEBUG_OUTPUT
	TBuf<256> unicodeBuf;
	unicodeBuf.Copy(aDes);
	RDebug::RawPrint(unicodeBuf);
#else
	RBusDevComm c=iSerialKeyboard->iDevComm;
	TRequestStatus s;
	c.Write(s,aDes);
	User::WaitForRequest(s);
#endif
	}

