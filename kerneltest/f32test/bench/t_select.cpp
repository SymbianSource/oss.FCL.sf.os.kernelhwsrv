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
// f32test\bench\t_select.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include "t_select.h"
//
LOCAL_D const TInt KArrayGranularity=16;
LOCAL_D const TInt KDriveSelector=0;

CSelectionBox* CSelectionBox::NewL(CConsoleBase* aConsole)
//
// Create a CSelection box
//
	{

	CSelectionBox* selBox=new(ELeave) CSelectionBox;
	CleanupStack::PushL(selBox);
	selBox->iText=new(ELeave) CArrayFixFlat<TSelBoxEntry>(KArrayGranularity);
	TSelBoxEntry firstEntry;
	firstEntry.iText=_L("Exit").AllocL();
	CleanupStack::PushL(firstEntry.iText);
	selBox->iText->InsertL(0,firstEntry);
	CleanupStack::Pop(2);
	selBox->iConsole=aConsole;
	return(selBox);
	}

CSelectionBox::~CSelectionBox()
//
// Destructor
//
	{

	if (iText==NULL)
		return;
	TInt count=iText->Count();
	while (count--)
		User::Free((*iText)[count].iText);
	delete iText;
	}

void CSelectionBox::AddLineL(const TDesC& aText,const TCallBack& aCallBack)
//
// Add a line to be displayed
//
	{
	
	TSelBoxEntry entry;
	entry.iText=aText.AllocL();
	CleanupStack::PushL(entry.iText);
	entry.iCallBack=aCallBack;
	iText->InsertL(iText->Count()-1,entry);
	CleanupStack::Pop();
	}

void CSelectionBox::ReplaceTextL(TInt aLine,const TDesC& aText)
//
// Replace text at aLine
//
	{

	TSelBoxEntry& entry=(*iText)[aLine];
	User::Free(entry.iText);
	entry.iText=aText.AllocL();
	}

void CSelectionBox::Display()
//
// Display the box
//
	{
	iConsole->ClearScreen();

	TSize size=iConsole->ScreenSize();
	TInt widestLine=0;
	TInt items=iText->Count();

	TInt count=items;
	while (count--)
		widestLine=Max(widestLine,(*iText)[count].iText->Length());

	iTopLeft=TPoint((size.iWidth-widestLine)/2,(size.iHeight-items)/2-2);
	TInt downOffset=iTopLeft.iY;

	for (count=0;count<items;count++)
		{
		if (count==iHighLight)
			DisplayHighLight(ETrue);
		iConsole->SetCursorPosAbs(TPoint(iTopLeft.iX,downOffset));
		iConsole->Printf(_L("%S"),(*iText)[count].iText);
		downOffset++;
		}

	iConsole->SetCursorPosAbs(TPoint(iTopLeft.iX-1,iTopLeft.iY+iHighLight));
	}

void CSelectionBox::DisplayHighLight(TBool aOn)
//
// Draw aHighLight to the left of aLine
//
	{

	TBuf<1> cursor(1);
	cursor[0]=(TUint8)((aOn) ? 16 : 32);
	iConsole->SetCursorPosAbs(TPoint(iTopLeft.iX-2,iTopLeft.iY+iHighLight));
	iConsole->Printf(_L("%S"),&cursor);
	}

TChar CSelectionBox::CurrentKeyPress()
	{return(iChar);}

TInt CSelectionBox::CurrentDrive()
	{return(iCurrentDrive);}

TBool CSelectionBox::MediaPresent(TInt aDrive)
//
// Return ETrue if a media is present in aDrive
//
	{

	TDriveInfo driveInfo;
	iFs.Drive(driveInfo,aDrive);
	if (driveInfo.iType!=EMediaNotPresent)
		return(ETrue);
	return(EFalse);
	}

TInt CSelectionBox::NextDrive(TInt aDrive)
//
// Find the next drive in the driveList
//
	{

	for(TInt i=aDrive+1;i<=EDriveZ;i++)
		{
		if (iDriveList[i]!=0 && MediaPresent(i))
			return(i);
		}
	return(aDrive);
	}
		
TInt CSelectionBox::PreviousDrive(TInt aDrive)
//
// Find the next drive in the driveList
//
	{

	for (TInt i=aDrive-1;i>=0;i--)
		{
		if (iDriveList[i]!=0 && MediaPresent(i))
			return(i);
		}
	return(aDrive);
	}

void CSelectionBox::SetDriveName()
//
// Set the drive name
//
	{

	TBuf<16> driveName;
	if (PreviousDrive(iCurrentDrive)!=iCurrentDrive)
		driveName+=_L("<-");
	driveName+=_L("Drive ");
	driveName.Append('A'+iCurrentDrive);
	driveName.Append(':');
	if (NextDrive(iCurrentDrive)!=iCurrentDrive)
		driveName+=_L("->");
	while (driveName.Length()<12)
		driveName+=_L(" ");
	(*iText)[0].iText->Des()=driveName;
	}

void CSelectionBox::DisplayDrive()
//
// Display the drive
//
	{

	iConsole->SetCursorPosAbs(iTopLeft);
	iConsole->Printf(_L("%S"),(*iText)[0].iText);
	iConsole->SetCursorPosAbs(TPoint(iTopLeft.iX-1,iTopLeft.iY));
	}

GLDEF_C TInt ChangeDrives(TAny* aSelector)
//
// Move to next drive if it is mounted
//
	{
	CSelectionBox& selBox=*(CSelectionBox*)aSelector;
	TChar key=selBox.CurrentKeyPress();
	TInt drive=selBox.iCurrentDrive;
	if (key==EKeyLeftArrow)
		drive=selBox.PreviousDrive(selBox.iCurrentDrive);
	else if (key==EKeyRightArrow)
		drive=selBox.NextDrive(selBox.iCurrentDrive);
	selBox.iCurrentDrive=drive;
	selBox.SetDriveName();
	selBox.DisplayDrive();
	return(KErrNone);
	}

void CSelectionBox::AddDriveSelectorL(RFs aFs)
//
// Add a drive selector to the list
//
	{

	if (iDriveSelectorPresent)
		return;
	iDriveSelectorPresent=ETrue;
	iFs=aFs;
	iFs.DriveList(iDriveList);
	iCurrentDrive=EDriveC;
	TSelBoxEntry entry;
	entry.iText=_L("<-Drive X:->").AllocL();
	CleanupStack::PushL(entry.iText);
	entry.iCallBack=TCallBack(ChangeDrives,this);
	iText->InsertL(0,entry);
	CleanupStack::Pop();
	SetDriveName();
	}

void CSelectionBox::Run()
//
// Display the box and handle keypresses
//
	{
	
	Display();

	FOREVER
		{
		iChar=iConsole->Getch();
		switch (iChar)
	    	{
		case EKeyEscape:
			return;
		case EKeyHome:
			DisplayHighLight(EFalse);
			iHighLight=KDriveSelector;
			DisplayHighLight(ETrue);
			break;
		case EKeyLeftArrow:
			(*iText)[iHighLight].iCallBack.CallBack();
			break;
		case EKeyRightArrow:
			(*iText)[iHighLight].iCallBack.CallBack();
			break;
		case EKeyUpArrow:
			if (iHighLight)
				{
				DisplayHighLight(EFalse);
				iHighLight--;
				DisplayHighLight(ETrue);
				}
			break;
		case EKeyDownArrow:
			if (iHighLight+1<iText->Count())
				{
				DisplayHighLight(EFalse);
				iHighLight++;
				DisplayHighLight(ETrue);
				}
			break;
		case EKeyEnter:
			if (iHighLight+1==iText->Count())
				return;
			if (iHighLight==KDriveSelector && iDriveSelectorPresent)
				break;
			iConsole->ClearScreen();
			(*iText)[iHighLight].iCallBack.CallBack();
			Display();
			break;
		default:
			break;
			}
		}
	}

