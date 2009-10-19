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
// e32\euser\cbase\ub_cons.cpp
// 
//

#include "ub_std.h"
#include <e32uid.h>
#include <e32wins.h>



/**
Default constructor.
*/
EXPORT_C CConsoleBase::CConsoleBase()
	{
	}



/**
Destructor.
*/
EXPORT_C CConsoleBase::~CConsoleBase()
	{
	}




/**
Gets a character from the console.

@return the key code from the console.
*/
EXPORT_C TKeyCode CConsoleBase::Getch()
	{

	TRequestStatus s;
	Read(s);
	User::WaitForRequest(s);
	__ASSERT_ALWAYS(s==KErrNone,Panic(EConsGetchFailed));
	return(KeyCode());
	}




/**
Prints characters to the console window.

@param aFmt             The  non-modifiable descriptor containing the
                        format string. The TRefByValue class provides a
                        constructor which takes a TDesC type. 

@param ...              A variable number of arguments to be converted to text
                        as dictated by the format string. 
*/
EXPORT_C void CConsoleBase::Printf(TRefByValue<const TDesC> aFmt,...)
	{

	TestOverflowTruncate overflow;
	// coverity[var_decl]
	VA_LIST list;
	VA_START(list,aFmt);
	TBuf<0x100> aBuf;
	// coverity[uninit_use_in_call]
	TRAP_IGNORE(aBuf.AppendFormatList(aFmt,list,&overflow)); // ignore leave in TTimeOverflowLeave::Overflow()
	Write(aBuf);
	}



/**
Sets the cursor's x-position.

@param aX The x-position.
*/
EXPORT_C void CConsoleBase::SetPos(TInt aX)
	{

	SetCursorPosAbs(TPoint(aX,WhereY()));
	}




/**
Sets the cursor's x-position and y-position.

@param aX The x-position.
@param aY The y-position.
*/
EXPORT_C void CConsoleBase::SetPos(TInt aX,TInt aY)
	{

	SetCursorPosAbs(TPoint(aX,aY));
	}




/**
Gets the cursor's x-position.

@return The cursor's x-position.
*/
EXPORT_C TInt CConsoleBase::WhereX() const
	{

	return(CursorPos().iX);
	}



/**
Gets the cursor's y-position.

@return The cursor's y-position.
*/
EXPORT_C TInt CConsoleBase::WhereY() const
	{

	return(CursorPos().iY);
	}


/**
Extension function


*/
EXPORT_C TInt CConsoleBase::Extension_(TUint aExtensionId, TAny*& a0, TAny* a1)
	{
	return CBase::Extension_(aExtensionId, a0, a1);
	}


void CColorConsoleBase::SetTextAttribute(TTextAttribute /*anAttribute*/)
//
//
//
	{
	}


/**
Extension function


*/
EXPORT_C TInt CColorConsoleBase::Extension_(TUint aExtensionId, TAny*& a0, TAny* a1)
	{
	return CConsoleBase::Extension_(aExtensionId, a0, a1);
	}


NONSHARABLE_CLASS(CProxyConsole) : public CColorConsoleBase
	{
public:
	~CProxyConsole();
	TInt Construct(const TDesC& aImplDll);
public:
// implement for CConsoleBase
	TInt Create(const TDesC &aTitle,TSize aSize);
	void Read(TRequestStatus &aStatus);
	void ReadCancel();
	void Write(const TDesC &aDes);
	TPoint CursorPos() const;
	void SetCursorPosAbs(const TPoint &aPoint);
	void SetCursorPosRel(const TPoint &aPoint);
	void SetCursorHeight(TInt aPercentage);
	void SetTitle(const TDesC &aTitle);
	void ClearScreen();
	void ClearToEndOfLine();
	TSize ScreenSize() const;
	TKeyCode KeyCode() const;
	TUint KeyModifiers() const;
// implement for CColorConsoleBase
	void SetTextAttribute(TTextAttribute anAttribute); 
	virtual TInt Extension_(TUint aExtensionId, TAny*& a0, TAny* a1);
private:
	RLibrary iLib;
	CColorConsoleBase* iConsole;
	};

TInt CProxyConsole::Construct(const TDesC& aImplDll)
	{
	const TUidType type(KNullUid, KSharedLibraryUid, KConsoleDllUid);
	TInt r=iLib.Load(aImplDll,type);
	if (r==KErrNone)
		{
		iConsole=(CColorConsoleBase*)(iLib.Lookup(1)());
		if (!iConsole)
			r=KErrNoMemory;
		}
	return r;
	}

CProxyConsole::~CProxyConsole()
	{
	delete iConsole;
	iLib.Close();
	}

TInt CProxyConsole::Create(const TDesC &aTitle,TSize aSize)
	{
	return iConsole->Create(aTitle,aSize);
	}
	
void CProxyConsole::Read(TRequestStatus &aStatus)
	{
	iConsole->Read(aStatus);
	}
	
void CProxyConsole::ReadCancel()
	{
	iConsole->ReadCancel();
	}
	
void CProxyConsole::Write(const TDesC &aDes)
	{
	iConsole->Write(aDes);
	}
	
TPoint CProxyConsole::CursorPos() const
	{
	return iConsole->CursorPos();
	}
	
void CProxyConsole::SetCursorPosAbs(const TPoint &aPoint)
	{
	iConsole->SetCursorPosAbs(aPoint);
	}
	
void CProxyConsole::SetCursorPosRel(const TPoint &aPoint)
	{
	iConsole->SetCursorPosRel(aPoint);
	}
	
void CProxyConsole::SetCursorHeight(TInt aPercentage)
	{
	iConsole->SetCursorHeight(aPercentage);
	}
	
void CProxyConsole::SetTitle(const TDesC &aTitle)
	{
	iConsole->SetTitle(aTitle);
	}
	
void CProxyConsole::ClearScreen()
	{
	iConsole->ClearScreen();
	}
	
void CProxyConsole::ClearToEndOfLine()
	{
	iConsole->ClearToEndOfLine();
	}
	
TSize CProxyConsole::ScreenSize() const
	{
	return iConsole->ScreenSize();
	}
	
TKeyCode CProxyConsole::KeyCode() const
	{
	return iConsole->KeyCode();
	}
	
TUint CProxyConsole::KeyModifiers() const
	{
	return iConsole->KeyModifiers();
	}
	
void CProxyConsole::SetTextAttribute(TTextAttribute anAttribute)
	{
	iConsole->SetTextAttribute(anAttribute);
	}

TInt CProxyConsole::Extension_(TUint aExtensionId, TAny*& a0, TAny* a1)
	{
	return iConsole->Extension_(aExtensionId, a0, a1);
	}


_LIT(KConsImpl,"econs");
#ifdef __WINS__
_LIT(KConsGuiImpl,"econseik");
_LIT(KConsNoGuiImpl,"econsnogui");
_LIT(KConsWservImpl, "econswserv");
#endif



/**
Creates a new console object.

@param aTitle The title text for the console.
              This should not be longer than 256 characters.
@param aSize  The size of the console window.

@return A pointer to the new console object.

@see CConsoleBase::Create() 
*/
EXPORT_C CConsoleBase *Console::NewL(const TDesC &aTitle,TSize aSize)
	{
	CProxyConsole *pC=new(ELeave) CProxyConsole;
	TInt r=pC->Construct(KConsImpl);
	if (r==KErrNone)
		r=pC->Create(aTitle,aSize);
#ifdef __WINS__
	if (r!=KErrNone)
		{
		delete pC;
		pC=new(ELeave) CProxyConsole;
		if (EmulatorNoGui())
			{
			// try and create a dummy console via ECONSNOGUI
			r=pC->Construct(KConsNoGuiImpl);
			}
		else if (EmulatorMiniGui())
			{
			// try and create Wserv console via ECONSWSERV
			r=pC->Construct(KConsWservImpl);
			}
		else
			{
			// try and create a GUI console via ECONSEIK instead
			r=pC->Construct(KConsGuiImpl);
			}
		if (r==KErrNone)
			r=pC->Create(aTitle,aSize);
		}
#endif
	if (r!=KErrNone)
		{
		delete pC;
		User::Leave(r);
		}
	return(pC);
	}
