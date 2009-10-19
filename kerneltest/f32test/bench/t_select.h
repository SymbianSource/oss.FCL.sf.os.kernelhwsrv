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
// f32test\bench\t_select.h
// 
//

#if !defined(__T_SELECT_H__)
#define __T_SELECT_H__
#if !defined(__E32BASE_H__)
#include <e32base.h>
#endif
#if !defined(__E32SVR_H__)
#include <e32svr.h>
#endif
//

class TSelBoxEntry
	{
public:
	HBufC* iText;
	TCallBack iCallBack;
	};

class CSelectionBox : public CBase
	{
public:
	static CSelectionBox* NewL(CConsoleBase* aConsole);
	~CSelectionBox();
	void AddDriveSelectorL(RFs aFs);
	void AddLineL(const TDesC& aText,const TCallBack& aCallBack);
	void ReplaceTextL(TInt aLine,const TDesC& aText);
	void Run();
	TChar CurrentKeyPress();
	TInt CurrentDrive();
private:
	void Display();
	void DisplayHighLight(TBool aOn);
	TInt PreviousDrive(TInt aDrive);
	TInt NextDrive(TInt aDrive);
	void SetDriveName();
	void DisplayDrive();
	TBool MediaPresent(TInt aDrive);
private:
	CConsoleBase* iConsole;
	CArrayFixFlat<TSelBoxEntry>* iText;
	TInt iHighLight;
	TPoint iTopLeft;
	TChar iChar;
	TDriveList iDriveList;
	TInt iCurrentDrive;
	RFs iFs;
	TBool iDriveSelectorPresent;
friend TInt ChangeDrives(TAny*);
	};

#endif
