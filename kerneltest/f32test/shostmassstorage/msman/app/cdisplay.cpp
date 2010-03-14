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
// USB Mass Storage Application - also used as an improvised boot loader mechanism
//
//



/**
 @file
*/

#include <e32cons.h>
#include <f32file.h>

#include "rusbotgsession.h"

#include "usbtypes.h"
#include "mdrivedisplay.h"
#include "cdisplay.h"

// Display positions and test constants

// Available drives
static const TInt KStartRow_AvailableDrives = 1;
_LIT(KAvailDriveMsg, "Drives: ");

// Number of attached devices
static const TInt KRow_DevicesNumber = 2;
_LIT(KMsg_DevicesAttached, "USB Devices Attached = %d");

// Device Map
static const TInt KStartRow_DeviceMap = KRow_DevicesNumber + 2;
static const TInt KMaxRows_DeviceMap = 4;
_LIT(KMsg_DeviceMap_DriveList, "%d: ");          // [drive index]
_LIT(KMsg_DeviceMap_DriveLunEntry, "%c ");       // [drive letter]

// Drive Map
static const TInt KStartRow_DriveMap = KStartRow_DeviceMap + KMaxRows_DeviceMap;
static const TInt KMaxRows_DriveMap = 4;

// Drive info
static const TInt KStartRow_MsgWindow = KStartRow_DriveMap + KMaxRows_DriveMap + 2;

_LIT(KMsg_DriveMap_EntryLetter, "%c token = %d");           // [drive letter] [token]
_LIT(KDbgMsg_DriveMap_EntryLetter, "*** %c token = %d");    // [drive letter] [token]
                                                            
static const TInt KRowScrollWindowStart = KStartRow_MsgWindow;    

// System Status
static TPoint KPointSystemStatus(5, 0);
_LIT(KMsg_UpTime, "up time     : %dh:%dm:%ds   ");	// use trailing space to overwrite any leftover chars in line

//static const TInt KStartRow_MemoryFree = 1 + KStartRow_SystemStatus;
static TPoint KPointMemoryFree(5, 1);
_LIT(KMsg_MemoryFree, "mem (bytes) : 0x%X");

// User Keys
static const TPoint KPointUser1Keys(5,2);
_LIT(KMsgUser1Keys, "[Esc]=Quit [A-Z]=DriveInfo");
static const TPoint KPointUser2Keys(5,3);
_LIT(KMsgUser2Keys, "[F5|SPACE]=Hub update");


// Scroll Window status
_LIT(KScrollWindowStatus, "Page %d of %d");


_LIT(KDriveAtts,"DriveList %c: %02x ");


// ****************************************************************************


CScrollWindow* CScrollWindow::NewL(CConsoleBase& aConsole, TInt aStartRow, TInt aEndRow)
    {
	CScrollWindow* r = new (ELeave) CScrollWindow(aConsole, aStartRow, aEndRow);
	CleanupStack::PushL(r);
	r->ConstructL();
    CleanupStack::Pop(r);
	return r;
    }


void CScrollWindow::ConstructL()
    {
    }

_LIT(KTxtPanic,"HUSBCONSAPP");

CScrollWindow::CScrollWindow(CConsoleBase& aConsole, TInt aStartRow, TInt aEndRow)
:   iConsole(aConsole),
    iStartRow(aStartRow),
    iEndRow(aEndRow),
    iPageLength(iEndRow - iStartRow)
    {
    __ASSERT_ALWAYS(iEndRow > iStartRow, User::Panic(KTxtPanic, -1));
    }

CScrollWindow::~CScrollWindow()
    {
    iLineArray.Close();
    }

void CScrollWindow::Reset()
    {
    iPage = 0;
    iLineArray.Reset();
    }


void CScrollWindow::AppendL(const TDesC& aLine)
    {
    iTmpLine.Zero();
    iLineArray.AppendL(iTmpLine);
    TInt last = iLineArray.Count() - 1;
    iLineArray[last].Copy(aLine);
    }


TLine* CScrollWindow::NewLineL()
    {
    iTmpLine.Zero();
    iLineArray.AppendL(iTmpLine);
    TInt last = iLineArray.Count() - 1;
    return &iLineArray[last];
    }


void CScrollWindow::Update()
    {
    TInt line = iPage * iPageLength;

    TInt row = iStartRow;
    do
        {
        iConsole.SetPos(0, row + line%iPageLength);
        if (line < iLineArray.Count())
            {
            iConsole.Printf(iLineArray[line]);
            }
        iConsole.ClearToEndOfLine();
        line++;
        }
    while (((line-1)%iPageLength) != (iPageLength - 1));

    iConsole.SetPos(0, iStartRow + iPageLength);
    iConsole.Printf(KScrollWindowStatus, iPage + 1, iLineArray.Count()/iPageLength + 1);
    }

void CScrollWindow::PageInc()
    {
    TInt lastPage = iLineArray.Count()/iPageLength;
    if (iPage == lastPage)
        {
        iPage = 0;
        }
    else
        {
        iPage++;
        }
    }


void CScrollWindow::PageDec()
    {
    if (iPage == 0)
        {
        TInt lastPage = iLineArray.Count()/iPageLength;
        iPage = lastPage;
        }
    else
        {
        iPage--;
        }
    }


CDisplay* CDisplay::NewLC(RFs& aFs, CConsoleBase& aConsole)
    {
	CDisplay* r = new (ELeave) CDisplay(aFs, aConsole);
	CleanupStack::PushL(r);
	r->ConstructL();
	return r;
    }


void CDisplay::ConstructL()
    {    
    iScrollWindow = CScrollWindow::NewL(iConsole, KRowScrollWindowStart, iScreenSize.iHeight - iFooterY - 4);
    }


CDisplay::CDisplay(RFs& aFs, CConsoleBase& aConsole)
:   iFs(aFs),
    iConsole(aConsole)
    {
    iConsole.ClearScreen();
    iScreenSize = iConsole.ScreenSize();
    // Origin of footer
    iPointFooter = TPoint(iFooterX, iScreenSize.iHeight - iFooterY - 2);
    }


CDisplay::~CDisplay()
    {
    delete iScrollWindow;
    }


void CDisplay::Menu()
    {
    SetFooterPos(KPointUser1Keys);
    iConsole.Printf(KMsgUser1Keys);
    SetFooterPos(KPointUser2Keys);
    iConsole.Printf(KMsgUser2Keys);
    iCursorPos = iConsole.CursorPos();
    }


void CDisplay::DriveListL() const
{
    TDriveList drivelist;
    TRAPD(err, iFs.DriveList(drivelist));
    if (err)
        {
        return;
        }
    // A TDriveList (the list of available drives), is an array of
    // 26 bytes. Each byte with a non zero value signifies that the
    // corresponding drive is available.
    TBuf<KDisplayWidth> iLineBuffer;
    iLineBuffer = KAvailDriveMsg;
    TChar driveLetter;

    for (TInt driveNumber = EDriveA; driveNumber <= EDriveZ;driveNumber++)
        {
        if (drivelist[driveNumber]) // if drive-list entry non-zero, drive is available
            {
            // overflow check
            if (iLineBuffer.Length() == iLineBuffer.MaxLength())
                {
                iLineBuffer[iLineBuffer.MaxLength() - 1] = '>';
                break;
                }

            User::LeaveIfError(iFs.DriveToChar(driveNumber,driveLetter));

            // The following line prints the drive letter followed by the hex value
            // of the integer indicating that drive's attributes
            RDebug::Print(KDriveAtts,TUint(driveLetter), drivelist[driveNumber]);
            iLineBuffer.Append(driveLetter);

            }
        }

	iConsole.SetPos(0, KStartRow_AvailableDrives);
    iConsole.Printf(iLineBuffer);
	iConsole.ClearToEndOfLine();
    CursorHome();
}


void CDisplay::DevicesNumber(TInt aDevicesNumber) const
    {
	iConsole.SetPos(0, KRow_DevicesNumber);
	iConsole.Printf(KMsg_DevicesAttached, aDevicesNumber);
	iConsole.ClearToEndOfLine();
    }


void CDisplay::DriveMapL(const TDriveMap& aDriveMap) const
    {
    TChar letter;

    TInt i = 0;

    // Output to debug port
    for (; i < aDriveMap.Count(); i++)
        {
        TToken token = aDriveMap[i];
        if (token)
            {
            User::LeaveIfError(iFs.DriveToChar(i, letter));
            RDebug::Print(KDbgMsg_DriveMap_EntryLetter, TUint(letter), token);
            }
        }

    // Output to console
    TInt row = KStartRow_DriveMap;
    for (i = (aDriveMap.Count() -1); i >= 0  &&  row < (KStartRow_DriveMap + KMaxRows_DriveMap); i--)
        {
        TToken token = aDriveMap[i];
        if (token)
            {
            User::LeaveIfError(iFs.DriveToChar(i, letter));
            iConsole.SetPos(0, row);
            iConsole.Printf(KMsg_DriveMap_EntryLetter, TUint(letter), token);
            iConsole.ClearToEndOfLine();
            row++;
            }
        }

    for (; row < KStartRow_DriveMap + KMaxRows_DriveMap; row++)
        {
        iConsole.SetPos(0, row);
        iConsole.ClearToEndOfLine();
        }
    }


void CDisplay::DeviceMapL(TInt aRow, TInt deviceIndex, const TDeviceMap& aDeviceMap) const
    {
    TChar letter;
    TInt drive;

    // Output to debug port
    RDebug::Printf("*** deviceIndex = %d", deviceIndex);
    for (TInt lunIndex = 0; lunIndex < 16; lunIndex++)
        {
        drive = aDeviceMap[lunIndex];

        if (drive == 0)
            break;

        User::LeaveIfError(iFs.DriveToChar(drive, letter));
        RDebug::Printf("*** drive=%d %c", drive, TUint(letter));
        }

    //  Output to console
    if (aRow >= KMaxRows_DeviceMap)
        {
        return;
        }
    RDebug::Printf("-----> Device MAP %x", deviceIndex);
    TInt row = KStartRow_DeviceMap + aRow;
    iConsole.SetPos(0, row);
    iConsole.Printf(KMsg_DeviceMap_DriveList, deviceIndex);

    for (TInt lunIndex = 0; lunIndex < 16; lunIndex++)
        {
        drive = aDeviceMap[lunIndex];

        if (drive == 0)
            break;

        User::LeaveIfError(iFs.DriveToChar(drive, letter));
        iConsole.Printf(KMsg_DeviceMap_DriveLunEntry, TUint(letter));
        iConsole.ClearToEndOfLine();
        }
    }


void CDisplay::DeviceMapClear(TInt aRow) const
    {
    TInt row = KStartRow_DeviceMap;

    if (aRow > KMaxRows_DeviceMap)
        return;

    for (row = KStartRow_DeviceMap + aRow; row < KStartRow_DeviceMap + KMaxRows_DeviceMap; row++)
        {
        iConsole.SetPos(0, row);
        iConsole.ClearToEndOfLine();
        }
    }


void CDisplay::GetDriveInfoL(TChar aChar)
    {
    iScrollWindow->Reset();

    TDriveInfo driveInfo;

    TInt driveNumber;
    User::LeaveIfError(iFs.CharToDrive(aChar, driveNumber));

    TLine* line;
    line = iScrollWindow->NewLineL();
    _LIT(KDrive,"Drive=%d %C");
    line->Format(KDrive, driveNumber, TInt(aChar.GetUpperCase()));

    iFs.Drive(driveInfo, driveNumber);
    if (driveInfo.iDriveAtt == KDriveAbsent)
        {
        _LIT(KTxt_MappingDriveError, "Drive absent !");
        iScrollWindow->AppendL(KTxt_MappingDriveError);
        return;
        }

    FormatDriveInfoL(driveInfo);

    TVolumeInfo volumeInfo;

    TInt err = iFs.Volume(volumeInfo, driveNumber);
    if (err != KErrNotReady)
        // Volume() returns KErrNotReady if no volume present.
        // In this case, check next drive number
        {
        FormatVolumeInfoL(volumeInfo);
        }
    }


void CDisplay::DriveInfo()
    {
    iScrollWindow->Update();
    CursorHome();
    }

void CDisplay::FormatDriveInfoL(const TDriveInfo& aDriveInfo)
    {
    // Append battery, media and drive information to aBuffer
    // Define descriptor constants using the _LIT macro
    _LIT(KDriveInfo1, "iType=%02x %02x iDriveAtt=%04x");
    _LIT(KDriveInfo2, "iMediaAtt=%02x");
    _LIT(KConnectionBusInternal,"Connection Bus Internal");
    _LIT(KConnectionBusUsb,"Connection Bus USB");
    _LIT(KConnectionBusUnknown,"Connection Bus Unknown");
    _LIT(KNotPresent,"No media present");
    _LIT(KFloppy,"Media is floppy disk");
    _LIT(KHard,"Media is hard disk");
    _LIT(KCDROM,"Media is CD-ROM");
    _LIT(KRam,"Media is RAM");
    _LIT(KFlash,"Media is flash");
    _LIT(KRom,"Media is ROM");
    _LIT(KRemote,"Media is remote");
    _LIT(KExternal,"Media is external");
    _LIT(KNANDFlash,"Media is NAND flash");
    _LIT(KUnknown,"Media unknown");
    _LIT(KDriveAtts,"Drive attributes:");
    _LIT(KLocal," local");
    _LIT(KROMDrive," ROM");
    _LIT(KRedirected," redirected");
    _LIT(KSubstituted," substituted");
    _LIT(KInternal," internal");
    _LIT(KRemovable," removable");
    _LIT(KMediaAtts,"Media attributes:");
    _LIT(KDynamic," dynamic");
    _LIT(KDual," dual-density");
    _LIT(KFormattable," formattable");
    _LIT(KLockable," lockable");
    _LIT(KLocked," locked");
    _LIT(KHasPassword," has password");
    _LIT(KWriteProtected," write-protected");

    TLine* line;
    line = iScrollWindow->NewLineL();
    line->Format(KDriveInfo1, TInt(aDriveInfo.iType), TInt(aDriveInfo.iConnectionBusType), TInt(aDriveInfo.iDriveAtt));

    line = iScrollWindow->NewLineL();
    line->Format(KDriveInfo2, TInt(aDriveInfo.iMediaAtt));

    line = iScrollWindow->NewLineL();
    switch (aDriveInfo.iConnectionBusType)
        {
        case EConnectionBusInternal:
            line->Append(KConnectionBusInternal);
            break;
        case EConnectionBusUsb:
            line->Append(KConnectionBusUsb);
            break;
        default:
            line->Append(KConnectionBusUnknown);
        }

    line = iScrollWindow->NewLineL();
    switch (aDriveInfo.iType)
            {
        case EMediaNotPresent:
            line->Append(KNotPresent);
            break;
        case EMediaFloppy:
            line->Append(KFloppy);
            break;
        case EMediaHardDisk:
            line->Append(KHard);
            break;
        case EMediaCdRom:
            line->Append(KCDROM);
            break;
        case EMediaRam:
            line->Append(KRam);
            break;
        case EMediaFlash:
            line->Append(KFlash);
            break;
        case EMediaRom:
            line->Append(KRom);
            break;
        case EMediaRemote:
            line->Append(KRemote);
            break;
        case EMediaNANDFlash:
            line->Append(KNANDFlash);
            break;
        default:
            line->Append(KUnknown);
        }

        // Drive Attributes
        line = iScrollWindow->NewLineL();
        line->Append(KDriveAtts);
        if (aDriveInfo.iDriveAtt & KDriveAttLocal)
            {
            line = iScrollWindow->NewLineL();
            line->Append(KLocal);
            }
        if (aDriveInfo.iDriveAtt & KDriveAttRom)
            {
            line = iScrollWindow->NewLineL();
            line->Append(KROMDrive);
            }
        if (aDriveInfo.iDriveAtt & KDriveAttRedirected)
            {
            line = iScrollWindow->NewLineL();
            line->Append(KRedirected);
            }
        if (aDriveInfo.iDriveAtt & KDriveAttSubsted)
            {
            line = iScrollWindow->NewLineL();
            line->Append(KSubstituted);
            }
        if (aDriveInfo.iDriveAtt & KDriveAttInternal)
            {
            line = iScrollWindow->NewLineL();
            line->Append(KInternal);
            }
        if (aDriveInfo.iDriveAtt & KDriveAttRemovable)
            {
            line = iScrollWindow->NewLineL();
            line->Append(KRemovable);
            }
        if (aDriveInfo.iDriveAtt & KDriveAttExternal)
            {
            line = iScrollWindow->NewLineL();
            line->Append(KExternal);
            }

        // Media Attributes
        line = iScrollWindow->NewLineL();
        line->Append(KMediaAtts);
        if (aDriveInfo.iMediaAtt & KMediaAttVariableSize)
            {
            line = iScrollWindow->NewLineL();
            line->Append(KDynamic);
            }
        if (aDriveInfo.iMediaAtt & KMediaAttDualDensity)
            {
            line = iScrollWindow->NewLineL();
            line->Append(KDual);
            }
        if (aDriveInfo.iMediaAtt & KMediaAttFormattable)
            {
            line = iScrollWindow->NewLineL();
            line->Append(KFormattable);
            }
        if (aDriveInfo.iMediaAtt & KMediaAttWriteProtected)
            {
            line = iScrollWindow->NewLineL();
            line->Append(KWriteProtected);
            }
        if (aDriveInfo.iMediaAtt & KMediaAttLockable)
            {
            line = iScrollWindow->NewLineL();
            line->Append(KLockable);
            }

        if (aDriveInfo.iMediaAtt & KMediaAttLocked)
            {
            line = iScrollWindow->NewLineL();
            line->Append(KLocked);
            }
        if (aDriveInfo.iMediaAtt & KMediaAttHasPassword)
            {
            line = iScrollWindow->NewLineL();
            line->Append(KHasPassword);
            }
    }

void CDisplay::FormatVolumeInfoL(const TVolumeInfo& aVolumeInfo)
    {
    // Append volume information to line
    _LIT(KUID,  "Unique ID:  0x%08X");
    _LIT(KSize, "Size:       0x%LX bytes");
    _LIT(KFree, "Free space: 0x%LX bytes");
    _LIT(KVolName, "Volume name: %S");
    TLine* line;
    line = iScrollWindow->NewLineL();
    line->Format(KUID, aVolumeInfo.iUniqueID);
    line = iScrollWindow->NewLineL();
    line->Format(KSize, aVolumeInfo.iSize);
    line = iScrollWindow->NewLineL();
    line->Format(KFree, aVolumeInfo.iFree);
    line = iScrollWindow->NewLineL();
    line->Format(KVolName, &aVolumeInfo.iName);
    }


void CDisplay::UpTime(TUint aUpTime) const
    {
    TUint totalMins = aUpTime/60;
    TUint totalHrs = totalMins/60;
    
    SetFooterPos(KPointSystemStatus);    
    iConsole.Printf(KMsg_UpTime, totalHrs, totalMins%60, aUpTime%60);
    CursorHome();
    }

void CDisplay::MemoryFree(TInt aBytes) const
    {
    SetFooterPos(KPointMemoryFree);	
	iConsole.Printf(KMsg_MemoryFree, aBytes);
    CursorHome();
    }


//////////////////////////////////////////////////////////////////////////////
//
// CMessageKeyProcessor
//
//////////////////////////////////////////////////////////////////////////////
CMessageKeyProcessor::CMessageKeyProcessor(CDisplay& aDisplay, RUsbOtgSession& aUsbOtgSession)
:   CActive(CActive::EPriorityUserInput),
    iDisplay(aDisplay),
    iUsbOtgSession(aUsbOtgSession)
	{
	}

CMessageKeyProcessor* CMessageKeyProcessor::NewLC(CDisplay& aDisplay, RUsbOtgSession& aUsbOtgSession)
	{
	CMessageKeyProcessor* self=new (ELeave) CMessageKeyProcessor(aDisplay, aUsbOtgSession);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}


void CMessageKeyProcessor::ConstructL()
	{
	// Add to active scheduler
	CActiveScheduler::Add(this);
	RequestCharacter();
	}


CMessageKeyProcessor::~CMessageKeyProcessor()
	{
	// Make sure we're cancelled
	Cancel();
	}

void CMessageKeyProcessor::DoCancel()
	{
	iDisplay.ReadCancel();
	}

void CMessageKeyProcessor::RunL()
	{
	  // Handle completed request
	ProcessKeyPressL(iDisplay.KeyCode());
	}

void CMessageKeyProcessor::RequestCharacter()
	{
	// A request is issued to the CConsoleBase to accept a
	// character from the keyboard.
	iDisplay.Read(iStatus);
	SetActive();
	}

void CMessageKeyProcessor::ProcessKeyPressL(TKeyCode aKeyCode)
	{
    TBool done = HandleKeyL(aKeyCode);

    if (done)
        {
        CActiveScheduler::Stop();
        return;
        }

    RequestCharacter();
	}


TBool CMessageKeyProcessor::HandleKeyL(TKeyCode aKeyCode)
    {
    TBool done = EFalse;

    if (TChar(aKeyCode).IsAlpha())
        {
        iDisplay.GetDriveInfoL(aKeyCode);
        iDisplay.DriveInfo();
        return done;
        }

    switch (aKeyCode)
        {
        case EKeyF5:
        case EKeySpace:
            {
            // Update USB status
            iUsbOtgSession.DeviceInserted();
            iDisplay.DriveListL();
            }
            break;

        case EKeyUpArrow:
        case EKeyPageUp:
        case '[':
            iDisplay.PageDec();
            iDisplay.DriveInfo();
            break;
        case EKeyDownArrow:
        case EKeyPageDown:
        case ']':
            iDisplay.PageInc();
            iDisplay.DriveInfo();
            break;
        case EKeyEscape:
            done = ETrue;
            break;
        default:
            break;
        }
    return done;
    }


