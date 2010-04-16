// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
//



/**
 @file
 @internalComponent
*/

#ifndef CDISPLAY_H
#define CDISPLAY_H


static const TInt KDisplayWidth = 100;
static const TInt KLineLength = KDisplayWidth;

typedef TBuf<KLineLength> TLine;


class CScrollWindow: public CBase
{
public:
    static CScrollWindow* NewL(CConsoleBase& aConsole, TInt aStartRow, TInt aEndRow);
    ~CScrollWindow();

private:
    CScrollWindow(CConsoleBase& aConsole, TInt aStartRow, TInt aEndRow);
    void ConstructL();

public:
    void Reset();
    void Update();
    void AppendL(const TDesC& aLine);
    TLine* NewLineL();

    void PageInc();
    void PageDec();
    void PageZero() {iPage = 0;}

private:
    CConsoleBase& iConsole;
    TLine iTmpLine;

    RArray<TLine> iLineArray;
    TInt iPage;
    const TInt iStartRow;
    const TInt iEndRow;
    const TInt iPageLength;
};



class CDisplay: public MDriveDisplay, public CBase
    {
public:
	static CDisplay* NewLC(RFs& aFs, CConsoleBase& aConsole);
	~CDisplay();

private:
	CDisplay(RFs& aFs, CConsoleBase& aConsole);
	void ConstructL();

public:
    void Menu();

    void DriveListL() const;
    void DevicesNumber(TInt aDevicesNumber) const;
    void DriveMapL(const TDriveMap& aDriveMap) const ;
    void DeviceMapL(TInt aRow, TInt deviceIndex, const TDeviceMap& aDeviceMap) const;
    void DeviceMapClear(TInt deviceIndex) const;

    void UpTime(TUint aUpTime) const;
    void MemoryFree(TInt aBytes) const;

    void GetDriveInfoL(TChar aChar);
    void DriveInfo();

    void Read(TRequestStatus &aStatus) {iConsole.Read(aStatus);}
    void ReadCancel() {iConsole.ReadCancel();}
    TKeyCode KeyCode() const {return iConsole.KeyCode();}

    void PageInc() {iScrollWindow->PageInc();}
    void PageDec() {iScrollWindow->PageDec();}
    void PageZero() {iScrollWindow->PageZero();}

private:
    void FormatDriveInfoL(const TDriveInfo& aDriveInfo);
    void FormatVolumeInfoL(const TVolumeInfo& aVolumeInfo);

    void CursorHome() const;

    void SetFooterPos(TPoint iPos) const;

private:    
    static const TInt iFooterX = 0;
    static const TInt iFooterY = 4;

    RFs& iFs;
    CConsoleBase& iConsole;

    TSize iScreenSize;
    TPoint iCursorPos;

    TPoint iPointFooter;

    CScrollWindow* iScrollWindow;
    };


inline void CDisplay::CursorHome() const
    {
    iConsole.SetPos(iCursorPos.iX, iCursorPos.iY);
    }

inline void CDisplay::SetFooterPos(TPoint iPos) const
    {
    TPoint pos = iPos + iPointFooter;
    iConsole.SetPos(pos.iX, pos.iY);
    }

class CMessageKeyProcessor : public CActive
	{
public:
	static CMessageKeyProcessor* NewLC(CDisplay& aDisplay, RUsbOtgSession& aUsbOtgSession);
	~CMessageKeyProcessor();

private:
	CMessageKeyProcessor(CDisplay& aDisplay, RUsbOtgSession& aUsbOtgSession);
	void ConstructL();

public:
	// Issue request
	void RequestCharacter();
	// Cancel request.
	// Defined as pure virtual by CActive;
	// implementation provided by this class.
	void DoCancel();
	// Service completed request.
	// Defined as pure virtual by CActive;
	// implementation provided by this class,
	void RunL();
	// Called from RunL() to handle the completed request
	void ProcessKeyPressL(TKeyCode aKeyCode);

private:
    TBool HandleKeyL(TKeyCode aKeyCode);

private:
	CDisplay& iDisplay;
    RUsbOtgSession& iUsbOtgSession;
	};




#endif
