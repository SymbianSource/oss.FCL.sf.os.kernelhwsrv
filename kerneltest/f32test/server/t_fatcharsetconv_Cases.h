// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_fatcharsetconv_cases.h
// 
//

// Define Test Cases
#ifndef T_FATCHARSETCONV_CASES_H
#define T_FATCHARSETCONV_CASES_H

#include "t_fatcharsetconv_aux.h"

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
extern CFileMan* gFileman;
extern RPointerArray<RFile>* gFileHandles;
extern TBool gAsynch;
extern TRequestStatus gStat;

TChar gFixedDriveValid 		= 'C';
TChar gFixedDriveInvalid 	= '?';
TChar gFixedDriveReadOnly 	= 'Z';
TChar gFixedDriveNotReady 	= 'A';

////////////////////////////////////////////////////////////
//	Test case definitions
////////////////////////////////////////////////////////////
static const TTestCaseUnitaryBasic gBasicUnitaryTestCases[] =
	{
	
//*******************************************************

	
	
//*********************only with alphabetic characters**********************************

//Cases for RFile::Create() with only alphabetic characters:

 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with alphabetic character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 
		{
		{1, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\1\\Src\\ABC(DE).TX", 
		(TText*)L"?:\\T_FCSC\\1\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with alphabetic character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{2, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\2\\Src\\ABC(DE).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\2\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with alphabetic character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{3, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\3\\Src\\ABC(DE).TXT", 
		(TText*)L"?:\\T_FCSC\\3\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with alphabetic character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{4, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\4\\Src\\ABCDEF(GH).TX", 
		(TText*)L"?:\\T_FCSC\\4\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with alphabetic character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{5, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\5\\Src\\ABCDEF(GH).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\5\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with alphabetic character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{6, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\6\\Src\\ABCDEF(GH).TXT", 
		(TText*)L"?:\\T_FCSC\\6\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with alphabetic character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{7, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\7\\Src\\ABC(DEF).TX", 
		(TText*)L"?:\\T_FCSC\\7\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with alphabetic character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{8, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\8\\Src\\ABC(DEF).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\8\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with alphabetic character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 	
		{
		{9, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\9\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\9\\Src\\", {EMPTY, EMPTY},
		},
		},
		
//Cases for RFs::IsValidName() with only alphabetic characters:	
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFs::IsValidName()
//! @SYMTestCaseDesc 1.Tests API with alphabetic character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{10, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\10\\Src\\ABC(DE).TX", 
		(TText*)L"?:\\T_FCSC\\10\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{11, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\11\\Src\\ABC(DE).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\11\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{12, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\12\\Src\\ABC(DE).TXT", 
		(TText*)L"?:\\T_FCSC\\12\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{13, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\13\\Src\\ABCDEF(GH).TX", 
		(TText*)L"?:\\T_FCSC\\13\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{14, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\14\\Src\\ABCDEF(GH).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\14\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{15, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\15\\Src\\ABCDEF(GH).TXT", 
		(TText*)L"?:\\T_FCSC\\15\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{16, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\16\\Src\\ABC(DEF).TX", 
		(TText*)L"?:\\T_FCSC\\16\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{17, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\17\\Src\\ABC(DEF).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\17\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{18, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\18\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\18\\Src\\", {EMPTY, EMPTY},
		},
		},
	
//Cases for RFs::ReadFileSection() with only alphabetic characters:	
		{
		{19, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\19\\Src\\ABC(DE).TX", 
		(TText*)L"?:\\T_FCSC\\19\\Src\\", {{0,EOB}, EMPTY},
		},
		},
		
		{
		{20, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\20\\Src\\ABC(DE).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\20\\Src\\", {{1,EOB}, EMPTY},
		},
		},
		
		{
		{21, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\21\\Src\\ABC(DE).TXT", 
		(TText*)L"?:\\T_FCSC\\21\\Src\\", {{2,EOB}, EMPTY},
		},
		},
		
		{
		{22, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\22\\Src\\ABCDEF(GH).TX", 
		(TText*)L"?:\\T_FCSC\\22\\Src\\", {{3,EOB}, EMPTY},
		},
		},
		
		{
		{23, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\23\\Src\\ABCDEF(GH).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\23\\Src\\", {{4,EOB}, EMPTY},
		},
		},
		
		{
		{24, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\24\\Src\\ABCDEF(GH).TXT", 
		(TText*)L"?:\\T_FCSC\\24\\Src\\", {{5,EOB}, EMPTY},
		},
		},
		
		{
		{25, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\25\\Src\\ABC(DEF).TX", 
		(TText*)L"?:\\T_FCSC\\25\\Src\\", {{6,EOB}, EMPTY},
		},
		},
		
		{
		{26, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\26\\Src\\ABC(DEF).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\26\\Src\\", {{7,EOB}, EMPTY},
		},
		},
		
		{
		{27, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\27\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\27\\Src\\", {{8,EOB}, EMPTY},
		},
		},
		
//Cases for RFs::Delete() with only alphabetic characters:		
		{
		{28, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\28\\Src\\ABC(DE).TX", 
		(TText*)L"?:\\T_FCSC\\28\\Src\\", {{0,EOB}, EMPTY},
		},
		},
		
		{
		{29, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\29\\Src\\ABC(DE).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\29\\Src\\", {{1,EOB}, EMPTY},
		},
		},
		
		{
		{30, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\30\\Src\\ABC(DE).TXT", 
		(TText*)L"?:\\T_FCSC\\30\\Src\\", {{2,EOB}, EMPTY},
		},
		},
		
		{
		{31, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\31\\Src\\ABCDEF(GH).TX", 
		(TText*)L"?:\\T_FCSC\\31\\Src\\", {{3,EOB}, EMPTY},
		},
		},
		
		{
		{32, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\32\\Src\\ABCDEF(GH).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\32\\Src\\", {{4,EOB}, EMPTY},
		},
		},
		
		{
		{33, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\33\\Src\\ABCDEF(GH).TXT", 
		(TText*)L"?:\\T_FCSC\\33\\Src\\", {{5,EOB}, EMPTY},
		},
		},
		
		{
		{34, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\34\\Src\\ABC(DEF).TX", 
		(TText*)L"?:\\T_FCSC\\34\\Src\\", {{6,EOB}, EMPTY},
		},
		},
		
		{
		{35, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\35\\Src\\ABC(DEF).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\35\\Src\\", {{7,EOB}, EMPTY},
		},
		},
		
		{
		{36, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\36\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\36\\Src\\", {{8,EOB}, EMPTY},
		},
		},
		
//Cases for RDir::Open() with only alphabetic characters:
	
		{
		{37, EOpenDir}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\37\\Src\\TESTDIR\\", 
		(TText*)L"?:\\T_FCSC\\37\\Src\\", {BLOCK12, EMPTY},
		},
		},
		
//Cases for RFs::RmDir() with only alphabetic characters:		
		{
		{38, ERemoveDir}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\38\\Src\\TESTDIR\\DIR2\\", 
		(TText*)L"?:\\T_FCSC\\38\\Src\\", {BLOCK12, EMPTY},
		},
		},

//Cases for RFs::IsFileInRom() with only alphabetic characters:			
		{
		{39, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\39\\Src\\ABC(DE).TX", 
		(TText*)L"?:\\T_FCSC\\39\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{40, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\40\\Src\\ABC(DE).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\40\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{41, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\41\\Src\\ABC(DE).TXT", 
		(TText*)L"?:\\T_FCSC\\41\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{42, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\42\\Src\\ABCDEF(GH).TX", 
		(TText*)L"?:\\T_FCSC\\42\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{43, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\43\\Src\\ABCDEF(GH).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\43\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{44, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\44\\Src\\ABCDEF(GH).TXT", 
		(TText*)L"?:\\T_FCSC\\44\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{45, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\45\\Src\\ABC(DEF).TX", 
		(TText*)L"?:\\T_FCSC\\45\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{46, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\46\\Src\\ABC(DEF).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\46\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{47, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\47\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\47\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
//Cases for RFile::Replace() with only alphabetic characters:	
		{
		{48, EReplaceFile},
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\48\\Src\\ABC(DE).TX", 
		(TText*)L"?:\\T_FCSC\\48\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{49, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\49\\Src\\ABC(DE).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\49\\Src\\", {EMPTY, EMPTY},
		},
		},

		{
		{50, EReplaceFile},
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\50\\Src\\ABC(DE).TXT", 
		(TText*)L"?:\\T_FCSC\\50\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{51, EReplaceFile},
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\51\\Src\\ABCDEF(GH).TX", 
		(TText*)L"?:\\T_FCSC\\51\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{52, EReplaceFile},
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\52\\Src\\ABCDEF(GH).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\52\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{53, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\53\\Src\\ABCDEF(GH).TXT", 
		(TText*)L"?:\\T_FCSC\\53\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{54, EReplaceFile},
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\54\\Src\\ABC(DEF).TX", 
		(TText*)L"?:\\T_FCSC\\54\\Src\\", {EMPTY, EMPTY},
		},
		},

		{
		{55, EReplaceFile},
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\55\\Src\\ABC(DEF).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\55\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{56, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\56\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\56\\Src\\", {EMPTY, EMPTY},
		},
		},  
			
//Cases for RFile::FullName(),RFile::Name(),RFs::RealName() with only alphabetic characters:			

		{
		{57, EOperateOnFileNames},
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\57\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\57\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{58, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\58\\Src\\ABC(DE).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\58\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{59, EOperateOnFileNames},
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\59\\Src\\ABC(DE).TXT", 
		(TText*)L"?:\\T_FCSC\\59\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{60, EOperateOnFileNames},
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\60\\Src\\ABCDEF(GH).TX", 
		(TText*)L"?:\\T_FCSC\\60\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{61, EOperateOnFileNames},
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\61\\Src\\ABCDEF(GH).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\61\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{62, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\62\\Src\\ABCDEF(GH).TXT", 
		(TText*)L"?:\\T_FCSC\\62\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{63, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\63\\Src\\ABC(DEF).TX", 
		(TText*)L"?:\\T_FCSC\\63\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{64, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\64\\Src\\ABC(DEF).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\64\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{65, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\65\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\65\\Src\\", {BLOCK01, EMPTY},
		},
		},

//Cases for RFile::FullName(),RFile::Name(),RFs::RealName() with only alphabetic characters:					
		{
		{66, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\66\\Src\\ABC(DE).TX", 
		(TText*)L"?:\\T_FCSC\\66\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{67, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\67\\Src\\ABC(DE).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\67\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{68, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\68\\Src\\ABC(DE).TXT", 
		(TText*)L"?:\\T_FCSC\\68\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{69, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\69\\Src\\ABCDEF(GH).TX", 
		(TText*)L"?:\\T_FCSC\\69\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{70, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\70\\Src\\ABCDEF(GH).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\70\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{71, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\71\\Src\\ABCDEF(GH).TXT", 
		(TText*)L"?:\\T_FCSC\\71\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{72, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\72\\Src\\ABC(DEF).TX", 
		(TText*)L"?:\\T_FCSC\\72\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{73, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\73\\Src\\ABC(DEF).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\73\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{74, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\74\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\74\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{75, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\75\\Src\\TESTDIR\\DIR2\\", 
		(TText*)L"?:\\T_FCSC\\75\\Src\\", {BLOCK12, EMPTY},
		},
		},
		
//Cases for RFs::Att() and RFs::SetAtt() with only alphabetic characters:					
		{
		{76, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\76\\Src\\ABC(DE).TX", 
		(TText*)L"?:\\T_FCSC\\76\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{77, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\77\\Src\\ABC(DE).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\77\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{78, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\78\\Src\\ABC(DE).TXT", 
		(TText*)L"?:\\T_FCSC\\78\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{79, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\79\\Src\\ABCDEF(GH).TX", 
		(TText*)L"?:\\T_FCSC\\79\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{80, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\80\\Src\\ABCDEF(GH).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\80\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{81, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\81\\Src\\ABCDEF(GH).TXT", 
		(TText*)L"?:\\T_FCSC\\81\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{82, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\82\\Src\\ABC(DEF).TX", 
		(TText*)L"?:\\T_FCSC\\82\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{83, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\83\\Src\\ABC(DEF).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\83\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{84, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\84\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\84\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{85, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\85\\Src\\TESTDIR\\DIR3\\", 
		(TText*)L"?:\\T_FCSC\\85\\Src\\", {BLOCK12, EMPTY},
		},
		},
				
//Cases for RFs::Entry() with only alphabetic characters:						
		{
		{86, ERFsEntry}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\86\\Src\\TESTDIR_FS\\", 
		(TText*)L"?:\\T_FCSC\\86\\Src\\", {BLOCK13, EMPTY},
		},
		},
		
//Cases for RFs::GetDir() with only alphabetic characters:						
		{
		{87, EGetDir}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\87\\Src\\TESTDIR_FS\\DIR2\\", 
		(TText*)L"?:\\T_FCSC\\87\\Src\\", {BLOCK13, EMPTY},
		},
		}, 
		
//Cases for RFs::MkDir() with only alphabetic characters:		
		{
		{88, EMkDir}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\88\\Src\\TESTDIR_FS\\", 
		(TText*)L"?:\\T_FCSC\\88\\Src\\", {EMPTY, EMPTY},
		},
		}, 
		
//Cases for RFs::MkDirAll() with only alphabetic characters:		
		{
		{89, EMkDirAll}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\89\\Src\\TESTDIR_FS\\DIR2\\", 
		(TText*)L"?:\\T_FCSC\\89\\Src\\", {EMPTY, EMPTY},
		},
		}, 

//Cases for RFile::Temp():	
 		{
		{90, EFileTemp}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\90\\Src\\", 
		(TText*)L"?:\\T_FCSC\\90\\Src\\", {EMPTY, EMPTY},
		},
		},
		

//*********************only with Unicode characters****************************************		
	
//Cases for RFile::Create() with Unicode characters:
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 	
		{
		{91, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\91\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\91\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{92, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\92\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\92\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{93, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\93\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\93\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{94, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\94\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\94\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{95, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\95\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\95\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{96, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\96\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\96\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{97, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\97\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\97\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 
		{
		{98, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\98\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\98\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{99, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\99\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\99\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 
		{
		{100, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\100\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\100\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{101, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\101\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\101\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{102, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\102\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\102\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{103, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\103\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\103\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{104, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\104\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\104\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{105, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\105\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\105\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{106, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\106\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\106\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{107, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\107\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\107\\Src\\", {EMPTY, EMPTY},
		},
		},
		
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 
		{
		{108, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\108\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\108\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{109, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\109\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\109\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Unicode character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{110, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\110\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\110\\Src\\", {EMPTY, EMPTY},
		},
		},
		
//Cases for RFs::IsValidName() with Unicode characters:		
			
		{
		{111, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\111\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\111\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{112, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\112\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\112\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{113, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\113\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\113\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{114, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\114\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\114\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{115, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\115\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\115\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{116, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\116\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\116\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{117, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\117\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\117\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{118, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\118\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\118\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{119, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\119\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\119\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{120, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\120\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\120\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{121, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\121\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\121\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{122, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\122\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\122\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{123, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\123\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\123\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{124, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\124\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\124\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{125, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\125\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\125\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{126, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\126\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\126\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{127, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\127\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\127\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{128, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\128\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\128\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{129, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\129\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\129\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{130, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\130\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\130\\Src\\", {EMPTY, EMPTY},
		},
		},
		
//Cases for RFs::ReadFileSection() with Unicode characters:	
		{
		{131, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\131\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\131\\Src\\", {{9,EOB}, EMPTY},
		},
		},
		
		{
		{132, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\132\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\132\\Src\\", {{10,EOB}, EMPTY},
		},
		},
		
		{
		{133, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\133\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\133\\Src\\", {{11,EOB}, EMPTY},
		},
		},
		
		{
		{134, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\134\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\134\\Src\\", {{12,EOB}, EMPTY},
		},
		},
		
		{
		{135, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\135\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\135\\Src\\", {{13,EOB}, EMPTY},
		},
		},
		
		{
		{136, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\136\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\136\\Src\\", {{14,EOB}, EMPTY},
		},
		},
		
		{
		{137, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\137\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\137\\Src\\", {{15,EOB}, EMPTY},
		},
		},
		
		{
		{138, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\138\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\138\\Src\\", {{16,EOB}, EMPTY},
		},
		},
		
		{
		{139, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\139\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\139\\Src\\", {{17,EOB}, EMPTY},
		},
		},
		
		{
		{140, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\140\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\140\\Src\\", {{18,EOB}, EMPTY},
		},
		},
		
		{
		{141, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\141\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\141\\Src\\", {{19,EOB}, EMPTY},
		},
		},
		
		{
		{142, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\142\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\142\\Src\\", {{20,EOB}, EMPTY},
		},
		},
		
		{
		{143, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\143\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\143\\Src\\", {{21,EOB}, EMPTY},
		},
		},
		
		{
		{144, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\144\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\144\\Src\\", {{22,EOB}, EMPTY},
		},
		},
		
		{
		{145, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\145\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\145\\Src\\", {{23,EOB}, EMPTY},
		},
		},
		
		{
		{146, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\146\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\146\\Src\\", {{24,EOB}, EMPTY},
		},
		},
		
		{
		{147, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\147\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\147\\Src\\", {{25,EOB}, EMPTY},
		},
		},
		
		{
		{148, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\148\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\148\\Src\\", {{26,EOB}, EMPTY},
		},
		},
		
		{
		{149, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\149\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\149\\Src\\", {{27,EOB}, EMPTY},
		},
		},
		
		{
		{150, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\150\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\150\\Src\\", {{28,EOB}, EMPTY},
		},
		},
		
//Cases for RFs::Delete() with Unicode characters:	
		{
		{151, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\151\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\151\\Src\\", {{9,EOB}, EMPTY},
		},
		},
		
		{
		{152, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\152\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\152\\Src\\", {{10,EOB}, EMPTY},
		},
		},
		
		{
		{153, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\153\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\153\\Src\\", {{11,EOB}, EMPTY},
		},
		},
		
		{
		{154, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\154\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\154\\Src\\", {{12,EOB}, EMPTY},
		},
		},
		
		{
		{155, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\155\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\155\\Src\\", {{13,EOB}, EMPTY},
		},
		},
		
		{
		{156, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\156\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\156\\Src\\", {{14,EOB}, EMPTY},
		},
		},
		
		{
		{157, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\157\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\157\\Src\\", {{15,EOB}, EMPTY},
		},
		},
		
		{
		{158, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\158\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\158\\Src\\", {{16,EOB}, EMPTY},
		},
		},
		
		{
		{159, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\159\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\159\\Src\\", {{17,EOB}, EMPTY},
		},
		},
		
		{
		{160, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\160\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\160\\Src\\", {{18,EOB}, EMPTY},
		},
		},
		
		{
		{161, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\161\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\161\\Src\\", {{19,EOB}, EMPTY},
		},
		},
		
		{
		{162, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\162\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\162\\Src\\", {{20,EOB}, EMPTY},
		},
		},
		
		{
		{163, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\163\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\163\\Src\\", {{21,EOB}, EMPTY},
		},
		},
		
		{
		{164, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\164\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\164\\Src\\", {{22,EOB}, EMPTY},
		},
		},
		
		{
		{165, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\165\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\165\\Src\\", {{23,EOB}, EMPTY},
		},
		},
		
		{
		{166, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\166\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\166\\Src\\", {{24,EOB}, EMPTY},
		},
		},
		
		{
		{167, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\167\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\167\\Src\\", {{25,EOB}, EMPTY},
		},
		},
		
		{
		{168, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\168\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\168\\Src\\", {{26,EOB}, EMPTY},
		},
		},
		
		{
		{169, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\169\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\169\\Src\\", {{27,EOB}, EMPTY},
		},
		},
		
		{
		{170, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\170\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\170\\Src\\", {{28,EOB}, EMPTY},
		},
		},

//Cases for RDir::Open() with Unicode characters:		
		{
		{171, EOpenDir}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\171\\Src\\\x65B0\x6587\\", 
		(TText*)L"?:\\T_FCSC\\171\\Src\\", {BLOCK17, EMPTY},
		},
		},
	
//Cases for RFs::RmDir() with Unicode characters:		
		{
		{172, ERemoveDir}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\172\\Src\\\x65B0\x6587\\\x65B0\x6587\x65B0\x6587\x65B0\\", 
		(TText*)L"?:\\T_FCSC\\172\\Src\\", {BLOCK17, EMPTY},
		},
		},

//Cases for RFs::IsFileInRom() with Unicode characters:
		{
		{173, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\173\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\173\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{174, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\174\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\174\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{175, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\175\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\175\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{176, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\176\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\176\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{177, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\177\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\177\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{178, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\178\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\178\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{179, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\179\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\179\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{180, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\180\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\180\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{181, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\181\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\181\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{182, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\182\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\182\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{183, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\183\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\183\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{184, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\184\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\184\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{185, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\185\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\185\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{186, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\186\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\186\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{187, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\187\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\187\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{188, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\188\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\188\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{189, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\189\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\189\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{190, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\190\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\190\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{191, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\191\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\191\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{192, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\192\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\192\\Src\\", {BLOCK06, EMPTY},
		},
		},
	
//Cases for RFile::Replace() with Unicode characters:	
		{
		{193, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\193\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\193\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{194, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\194\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\194\\Src\\", {EMPTY, EMPTY},
		},
		},

		{
		{195, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\195\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\195\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{196, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\196\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\196\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{197, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\197\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\197\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{198, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\198\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\198\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{199, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\199\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\199\\Src\\", {EMPTY, EMPTY},
		},
		},

		{
		{200, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\200\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\200\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{201, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\201\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\201\\Src\\", {EMPTY, EMPTY},
		},
		}, 
		
		{
		{202, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\202\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\202\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{203, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\203\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\203\\Src\\", {EMPTY, EMPTY},
		},
		},

		{
		{204, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\204\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\204\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{205, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\205\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\205\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{206, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\206\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\206\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{207, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\207\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\207\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{208, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\208\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\208\\Src\\", {EMPTY, EMPTY},
		},
		},

		{
		{209, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\209\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\209\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{210, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\210\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\210\\Src\\", {EMPTY, EMPTY},
		},
		}, 
		
		{
		{211, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\211\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\211\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{212, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\212\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\212\\Src\\", {EMPTY, EMPTY},
		},
		},
		
//Cases for RFile::FullName(),RFile::Name(),RFs::RealName() with Unicode characters:					
		{
		{213, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\213\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\213\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{214, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\214\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\214\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{215, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\215\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\215\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{216, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\216\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\216\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{217, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\217\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\217\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{218, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\218\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\218\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{219, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\219\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\219\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{220, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\220\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\220\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{221, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\221\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\221\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{222, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\222\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\222\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{223, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\223\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\223\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{224, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\224\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\224\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{225, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\225\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\225\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{226, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\226\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\226\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{227, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\227\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\227\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{228, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\228\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\228\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{229, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\229\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\229\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{230, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\230\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\230\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{231, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\231\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\231\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{232, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\232\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\232\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
//Cases for RFile::FullName(),RFile::Name(),RFs::RealName() with Unicode characters:					
		{
		{233, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\233\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\233\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{234, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\234\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\234\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{235, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\235\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\235\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{236, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\236\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\236\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{237, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\237\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\237\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{238, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\238\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\238\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{239, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\239\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\239\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{240, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\240\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\240\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{241, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\241\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\241\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{242, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\242\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\242\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{243, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\243\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\243\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{244, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\244\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\244\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{245, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\245\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\245\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{246, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\246\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\246\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{247, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\247\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\247\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{248, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\248\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\248\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{249, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\249\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\249\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{250, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\250\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\250\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{251, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\251\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\251\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{252, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\252\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\252\\Src\\", {BLOCK06, EMPTY},
		},
		},

		{
		{253, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\253\\Src\\\x65B0\x6587\\\x65B0\x6587\x65B0\x6587\\", 
		(TText*)L"?:\\T_FCSC\\253\\Src\\", {BLOCK17, EMPTY},
		},
		},

//Cases for RFs::Att() and RFs::SetAtt() with Unicode characters:		
		{
		{254, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\254\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\254\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{255, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\255\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\255\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{256, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\256\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\256\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{257, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\257\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\257\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{258, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\258\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\258\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{259, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\259\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\259\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{260, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\260\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\260\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{261, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\261\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\261\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{262, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\262\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\262\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{263, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\263\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\263\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{264, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\264\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\264\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{265, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\265\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\265\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{266, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\266\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\266\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{267, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\267\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\267\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{268, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\268\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\268\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{269, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\269\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\269\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{270, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\270\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\270\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{271, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\271\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\271\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{272, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\272\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\272\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{273, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\273\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\273\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{274, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\274\\Src\\\x65B0\x6587\\\x65B0\x6587\x65B0\x6587\x65B0\x6587\\", 
		(TText*)L"?:\\T_FCSC\\274\\Src\\", {BLOCK17, EMPTY},
		},
		},
		
//Cases for RFs::Entry() with Unicode characters:	
		{
		{275, ERFsEntry}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\275\\Src\\\x65B0\x6587\x4EF6\\", 
		(TText*)L"?:\\T_FCSC\\275\\Src\\", {BLOCK18, EMPTY},
		},
		},
		
//Cases for RFs::GetDir() with Unicode characters:	
		{
		{276, EGetDir}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\276\\Src\\\x65B0\x6587\x4EF6\\\x65B0\x4EF6\x65B0\x4EF6\\", 
		(TText*)L"?:\\T_FCSC\\276\\Src\\", {BLOCK18, EMPTY},
		},
		},	
			
//Cases for RFs::MkDir() with Unicode characters:		
		{
		{277, EMkDir}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\277\\Src\\\x65B0\x6587\x4EF6\\", 
		(TText*)L"?:\\T_FCSC\\277\\Src\\", {EMPTY, EMPTY},
		},
		},
				
//Cases for RFs::MkDirAll() with Unicode characters:		
		{
		{278, EMkDirAll}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\278\\Src\\\x65B0\x6587\x4EF6\\\x65B0\x4EF6\x65B0\x4EF6\x6587\\", 
		(TText*)L"?:\\T_FCSC\\278\\Src\\", {EMPTY, EMPTY},
		},
		},
		
//*********************Mixed(alpha and unicode characters**********************************		

//Cases for RFile::Create() with Mixed characters:

 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 	
		{
		{279, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\279\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\279\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 			
		{
		{280, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\280\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\280\\Src\\", {EMPTY, EMPTY},
		},
		},	
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{281, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\281\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\281\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{282, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\282\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\282\\Src\\", {EMPTY, EMPTY},
		},
		}, 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{283, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\283\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\283\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{284, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\284\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\284\\Src\\", {EMPTY, EMPTY},
		},
		},  
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{285, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\285\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\285\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{286, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\286\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\286\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{287, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\287\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\287\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{288, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\288\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\288\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 			
		{
		{289, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\289\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\289\\Src\\", {EMPTY, EMPTY},
		},
		},	
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{290, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\290\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\290\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{291, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\291\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\291\\Src\\", {EMPTY, EMPTY},
		},
		}, 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{292, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\292\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\292\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{293, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\293\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\293\\Src\\", {EMPTY, EMPTY},
		},
		},  
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{294, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\294\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\294\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{295, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\295\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\295\\Src\\", {EMPTY, EMPTY},
		},
		},
		
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 
		{
		{296, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\296\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\296\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 
		{
		{297, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\297\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\297\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{298, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\298\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\298\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{299, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\299\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\299\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{300, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\300\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\300\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{301, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\301\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\301\\Src\\", {EMPTY, EMPTY},
		},
		},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 		
		{
		{302, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\302\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\302\\Src\\", {EMPTY, EMPTY},
		},
		},
		
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_
//! @SYMTestType CIT 
//!
//! @SYMAPI RFile::Create()
//! @SYMTestCaseDesc 1.Tests API with Mixed character as input. 
//! @SYMTestActions Creates the file successfully.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 
		{
		{303, ECreateFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\303\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\303\\Src\\", {EMPTY, EMPTY},
		},
		},		

//Cases for RFs::IsValidName() with Mixed characters:	
		{
		{304, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\304\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\304\\Src\\", {EMPTY, EMPTY},
		},
		},
			
		{
		{305, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\305\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\305\\Src\\", {EMPTY, EMPTY},
		},
		},	
		
		{
		{306, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\306\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\306\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{307, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\307\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\307\\Src\\", {EMPTY, EMPTY},
		},
		}, 
		
		{
		{308, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\308\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\308\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{309, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\309\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\309\\Src\\", {EMPTY, EMPTY},
		},
		},  
		
		{
		{310, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\310\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\310\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{311, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\311\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\311\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{312, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\312\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\312\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{313, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\313\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\313\\Src\\", {EMPTY, EMPTY},
		},
		},
			
		{
		{314, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\314\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\314\\Src\\", {EMPTY, EMPTY},
		},
		},	
		
		{
		{315, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\315\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\315\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{316, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\316\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\316\\Src\\", {EMPTY, EMPTY},
		},
		}, 
		
		{
		{317, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\317\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\317\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{318, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\318\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\318\\Src\\", {EMPTY, EMPTY},
		},
		},  
		
		{
		{319, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\319\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\319\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{320, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\320\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\320\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{321, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\321\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\321\\Src\\", {EMPTY, EMPTY},
		},
		},

		{
		{322, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\322\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\322\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{323, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\323\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\323\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{324, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\324\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\324\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{325, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\325\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\325\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{326, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\326\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\326\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{327, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\327\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\327\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{328, EIsValidName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\328\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\328\\Src\\", {EMPTY, EMPTY},
		},
		},
		
//Cases for RFs::ReadFileSection() with Mixed characters:
		{
		{329, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\329\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\329\\Src\\", {{29,EOB}, EMPTY},
		},
		},
			
		{
		{330, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\330\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\330\\Src\\", {{30,EOB}, EMPTY},
		},
		},	
		
		{
		{331, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\331\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\331\\Src\\", {{31,EOB}, EMPTY},
		},
		},
		
		{
		{332, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\332\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\332\\Src\\", {{32,EOB}, EMPTY},
		},
		}, 
		
		{
		{333, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\333\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\333\\Src\\", {{33,EOB}, EMPTY},
		},
		},
		
		{
		{334, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\334\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\334\\Src\\", {{34,EOB}, EMPTY},
		},
		},  
		
		{
		{335, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\335\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\335\\Src\\", {{35,EOB}, EMPTY},
		},
		},
		
		{
		{336, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\336\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\336\\Src\\", {{36,EOB}, EMPTY},
		},
		},
		
		{
		{337, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\337\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\337\\Src\\", {{37,EOB}, EMPTY},
		},
		},
		
		{
		{338, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\338\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\338\\Src\\", {{38,EOB}, EMPTY},
		},
		},
			
		{
		{339, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\339\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\339\\Src\\", {{39,EOB}, EMPTY},
		},
		},	
		
		{
		{340, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\340\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\340\\Src\\", {{40,EOB}, EMPTY},
		},
		},
		
		{
		{341, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\341\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\341\\Src\\", {{41,EOB}, EMPTY},
		},
		}, 
		
		{
		{342, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\342\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\342\\Src\\", {{42,EOB}, EMPTY},
		},
		},
		
		{
		{343, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\343\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\343\\Src\\", {{43,EOB}, EMPTY},
		},
		},  
		
		{
		{344, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\344\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\344\\Src\\", {{44,EOB}, EMPTY},
		},
		},
		
		{
		{345, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\345\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\345\\Src\\", {{45,EOB}, EMPTY},
		},
		},
		
		{
		{346, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\346\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\346\\Src\\", {{46,EOB}, EMPTY},
		},
		},

		{
		{347, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\347\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\347\\Src\\", {{47,EOB}, EMPTY},
		},
		},
		
		{
		{348, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\348\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\348\\Src\\", {{48,EOB}, EMPTY},
		},
		},
		
		{
		{349, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\349\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\349\\Src\\", {{49,EOB}, EMPTY},
		},
		},
		
		{
		{350, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\350\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\350\\Src\\", {{50,EOB}, EMPTY},
		},
		},
		
		{
		{351, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\351\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\351\\Src\\", {{51,EOB}, EMPTY},
		},
		},
		
		{
		{352, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\352\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\352\\Src\\", {{52,EOB}, EMPTY},
		},
		},
		
		{
		{353, EReadFileSection}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\353\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\353\\Src\\", {{53,EOB}, EMPTY},
		},
		},
		
//Cases for RFs::Delete() with only Mixed characters:
		{
		{354, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\354\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\354\\Src\\", {{29,EOB}, EMPTY},
		},
		},
			
		{
		{355, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\355\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\355\\Src\\", {{30,EOB}, EMPTY},
		},
		},	
		
		{
		{356,EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\356\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\356\\Src\\", {{31,EOB}, EMPTY},
		},
		},
		
		{
		{357,EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\357\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\357\\Src\\", {{32,EOB}, EMPTY},
		},
		}, 
		
		{
		{358, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\358\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\358\\Src\\", {{33,EOB}, EMPTY},
		},
		},
		
		{
		{359, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\359\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\359\\Src\\", {{34,EOB}, EMPTY},
		},
		},  
		
		{
		{360, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\360\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\360\\Src\\", {{35,EOB}, EMPTY},
		},
		},
		
		{
		{361, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\361\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\361\\Src\\", {{36,EOB}, EMPTY},
		},
		},
		
		{
		{362, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\362\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\362\\Src\\", {{37,EOB}, EMPTY},
		},
		},
		
		{
		{363, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\363\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\363\\Src\\", {{38,EOB}, EMPTY},
		},
		},
			
		{
		{364, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\364\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\364\\Src\\", {{39,EOB}, EMPTY},
		},
		},	
		
		{
		{365, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\365\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\365\\Src\\", {{40,EOB}, EMPTY},
		},
		},
		
		{
		{366, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\366\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\366\\Src\\", {{41,EOB}, EMPTY},
		},
		}, 
		
		{
		{367, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\367\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\367\\Src\\", {{42,EOB}, EMPTY},
		},
		},
		
		{
		{368, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\368\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\368\\Src\\", {{43,EOB}, EMPTY},
		},
		},  
		
		{
		{369, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\369\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\369\\Src\\", {{44,EOB}, EMPTY},
		},
		},
		
		{
		{370, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\370\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\370\\Src\\", {{45,EOB}, EMPTY},
		},
		},
		
		{
		{371, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\371\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\371\\Src\\", {{46,EOB}, EMPTY},
		},
		},

		{
		{372, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\372\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\372\\Src\\", {{47,EOB}, EMPTY},
		},
		},
		
		{
		{373, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\373\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\373\\Src\\", {{48,EOB}, EMPTY},
		},
		},
		
		{
		{374, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\374\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\374\\Src\\", {{49,EOB}, EMPTY},
		},
		},
		
		{
		{375, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\375\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\375\\Src\\", {{50,EOB}, EMPTY},
		},
		},
		
		{
		{376, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\376\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\376\\Src\\", {{51,EOB}, EMPTY},
		},
		},
		
		{
		{377, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\377\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\377\\Src\\", {{52,EOB}, EMPTY},
		},
		},
		
		{
		{378, EDeleteFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\378\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\378\\Src\\", {{53,EOB}, EMPTY},
		},
		},		

//Cases for RDir::Open() with Mixed characters:
		{
		{379, EOpenDir}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\379\\Src\\\x65B0(A)\x6587\\", 
		(TText*)L"?:\\T_FCSC\\379\\Src\\", {BLOCK20, EMPTY},
		},
		},
		
//Cases for Rfs::RmDir() with Mixed characters:	
		{
		{380, ERemoveDir}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\380\\Src\\\x65B0\x6587(AB)\\\x65B0\x4EF6(AB)\x4EF6\\", 
		(TText*)L"?:\\T_FCSC\\380\\Src\\", {BLOCK21, EMPTY},
		},
		},
	
//Cases for RFs::IsFileInRom() with Mixed characters:	
		{
		{381, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\381\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\381\\Src\\", {BLOCK07, EMPTY},
		},
		},
			
		{
		{382, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\382\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\382\\Src\\", {BLOCK07, EMPTY},
		},
		},	
		
		{
		{383,EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\383\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\383\\Src\\", {BLOCK07, EMPTY},
		},
		},
		
		{
		{384,EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\384\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\384\\Src\\", {BLOCK07, EMPTY},
		},
		}, 
		
		{
		{385, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\385\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\385\\Src\\", {BLOCK07, EMPTY},
		},
		},
		
		{
		{386, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\386\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\386\\Src\\", {BLOCK08, EMPTY},
		},
		},  
		
		{
		{387, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\387\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\387\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{388, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\388\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\388\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{389, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\389\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\389\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{390, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\390\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\390\\Src\\", {BLOCK08, EMPTY},
		},
		},
			
		{
		{391, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\391\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\391\\Src\\", {BLOCK09, EMPTY},
		},
		},	
		
		{
		{392, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\392\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\392\\Src\\", {BLOCK09, EMPTY},
		},
		},
		
		{
		{393, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\393\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\393\\Src\\", {BLOCK09, EMPTY},
		},
		}, 
		
		{
		{394, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\394\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\394\\Src\\", {BLOCK09, EMPTY},
		},
		},
		
		{
		{395, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\395\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\395\\Src\\", {BLOCK09, EMPTY},
		},
		},  
		
		{
		{396, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\396\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\396\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{397, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\397\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\397\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{398, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\398\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\398\\Src\\", {BLOCK10, EMPTY},
		},
		},

		{
		{399, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\399\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\399\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{400, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\400\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\400\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{401, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\401\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\401\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{402, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\402\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\402\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{403, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\403\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\403\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{404, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\404\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\404\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{405, EIsFileInRom}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\405\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\405\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
//Cases for RFile::Replace() with Mixed characters:		
		{
		{406, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\406\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\406\\Src\\", {EMPTY, EMPTY},
		},
		},
			
		{
		{407, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\407\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\407\\Src\\", {EMPTY, EMPTY},
		},
		},	
		
		{
		{408, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\408\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\408\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{409, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\409\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\409\\Src\\", {EMPTY, EMPTY},
		},
		}, 
		
		{
		{410, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\410\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\410\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{411, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\411\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\411\\Src\\", {EMPTY, EMPTY},
		},
		},  
		
		{
		{412, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\412\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\412\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{413, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\413\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\413\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{414, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\414\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\414\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{415, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\415\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\415\\Src\\", {EMPTY, EMPTY},
		},
		},
			
		{
		{416, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\416\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\416\\Src\\", {EMPTY, EMPTY},
		},
		},	
		
		{
		{417, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\417\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\417\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{418, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\418\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\418\\Src\\", {EMPTY, EMPTY},
		},
		}, 
		
		{
		{419, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\419\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\419\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{420, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\420\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\420\\Src\\", {EMPTY, EMPTY},
		},
		},  
		
		{
		{421, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\421\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\421\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{422, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\422\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\422\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{423, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\423\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\423\\Src\\", {EMPTY, EMPTY},
		},
		},

		{
		{424, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\424\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\424\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{425, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\425\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\425\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{426, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\426\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\426\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{427, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\427\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\427\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{428, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\428\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\428\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{429, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\429\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\429\\Src\\", {EMPTY, EMPTY},
		},
		},
		
		{
		{430, EReplaceFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\430\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\430\\Src\\", {EMPTY, EMPTY},
		},
		},
				
//Cases for RFile::FullName(),RFile::Name(),RFs::RealName() with Mixed characters:					
		{
		{431, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\431\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\431\\Src\\", {BLOCK07, EMPTY},
		},
		},
			
		{
		{432, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\432\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\432\\Src\\", {BLOCK07, EMPTY},
		},
		},	
		
		{
		{433,EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\433\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\433\\Src\\", {BLOCK07, EMPTY},
		},
		},
		
		{
		{434,EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\434\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\434\\Src\\", {BLOCK07, EMPTY},
		},
		}, 
		
		{
		{435, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\435\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\435\\Src\\", {BLOCK07, EMPTY},
		},
		},
		
		{
		{436, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\436\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\436\\Src\\", {BLOCK08, EMPTY},
		},
		},  
		
		{
		{437, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\437\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\437\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{438, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\438\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\438\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{439, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\439\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\439\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{440, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\440\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\440\\Src\\", {BLOCK08, EMPTY},
		},
		},
			
		{
		{441, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\441\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\441\\Src\\", {BLOCK09, EMPTY},
		},
		},	
		
		{
		{442, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\442\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\442\\Src\\", {BLOCK09, EMPTY},
		},
		},
		
		{
		{443, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\443\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\443\\Src\\", {BLOCK09, EMPTY},
		},
		}, 
		
		{
		{444, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\444\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\444\\Src\\", {BLOCK09, EMPTY},
		},
		},
		
		{
		{445, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\445\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\445\\Src\\", {BLOCK09, EMPTY},
		},
		},  
		
		{
		{446, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\446\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\446\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{447, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\447\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\447\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{448, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\448\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\448\\Src\\", {BLOCK10, EMPTY},
		},
		},

		{
		{449, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\449\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\449\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{450, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\450\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\450\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{451, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\451\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\451\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{452, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\452\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\452\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{453, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\453\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\453\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{454, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\454\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\454\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{455, EOperateOnFileNames}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\455\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\455\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
//Cases for RFile::FullName(),RFile::Name(),RFs::RealName() with only alphabetic characters:						
		{
		{456, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\456\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\456\\Src\\", {BLOCK07, EMPTY},
		},
		},
			
		{
		{457, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\457\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\457\\Src\\", {BLOCK07, EMPTY},
		},
		},	
		
		{
		{458,EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\458\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\458\\Src\\", {BLOCK07, EMPTY},
		},
		},
		
		{
		{459,EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\459\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\459\\Src\\", {BLOCK07, EMPTY},
		},
		}, 
		
		{
		{460, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\460\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\460\\Src\\", {BLOCK07, EMPTY},
		},
		},
		
		{
		{461, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\461\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\461\\Src\\", {BLOCK08, EMPTY},
		},
		},  
		
		{
		{462, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\462\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\462\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{463, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\463\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\463\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{464, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\464\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\464\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{465, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\465\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\465\\Src\\", {BLOCK08, EMPTY},
		},
		},
			
		{
		{466, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\466\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\466\\Src\\", {BLOCK09, EMPTY},
		},
		},	
		
		{
		{467, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\467\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\467\\Src\\", {BLOCK09, EMPTY},
		},
		},
		
		{
		{468, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\468\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\468\\Src\\", {BLOCK09, EMPTY},
		},
		}, 
		
		{
		{469, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\469\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\469\\Src\\", {BLOCK09, EMPTY},
		},
		},
		
		{
		{470, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\470\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\470\\Src\\", {BLOCK09, EMPTY},
		},
		},  
		
		{
		{471, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\471\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\471\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{472, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\472\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\472\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{473, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\473\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\473\\Src\\", {BLOCK10, EMPTY},
		},
		},

		{
		{474, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\474\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\474\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{475, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\475\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\475\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{476, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\476\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\476\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{477, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\477\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\477\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{478, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\478\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\478\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{479, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\479\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\479\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{480, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\480\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\480\\Src\\", {BLOCK11, EMPTY},
		},
		},
				

		{
		{481, EFileModify}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\481\\Src\\\x65B0(A)\x6587\\\x65B0(ABCD)\x65B0\\", 
		(TText*)L"?:\\T_FCSC\\481\\Src\\", {BLOCK20, EMPTY},
		},
		},
		
//Cases for RFs::Att() and RFs::SetAtt() with Mixed characters:
		{
		{482, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\482\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\482\\Src\\", {BLOCK07, EMPTY},
		},
		},
			
		{
		{483, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\483\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\483\\Src\\", {BLOCK07, EMPTY},
		},
		},	
		
		{
		{484,EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\484\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\484\\Src\\", {BLOCK07, EMPTY},
		},
		},
		
		{
		{485,EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\485\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\485\\Src\\", {BLOCK07, EMPTY},
		},
		}, 
		
		{
		{486, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\486\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\486\\Src\\", {BLOCK07, EMPTY},
		},
		},
		
		{
		{487, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\487\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\487\\Src\\", {BLOCK08, EMPTY},
		},
		},  
		
		{
		{488, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\488\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\488\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{489, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\489\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\489\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{490, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\490\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\490\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{491, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\491\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\491\\Src\\", {BLOCK08, EMPTY},
		},
		},
			
		{
		{492, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\492\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\492\\Src\\", {BLOCK09, EMPTY},
		},
		},	
		
		{
		{493, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\493\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\493\\Src\\", {BLOCK09, EMPTY},
		},
		},
		
		{
		{494, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\494\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\494\\Src\\", {BLOCK09, EMPTY},
		},
		}, 
		
		{
		{495, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\495\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\495\\Src\\", {BLOCK09, EMPTY},
		},
		},
		
		{
		{496, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\496\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\496\\Src\\", {BLOCK09, EMPTY},
		},
		},  
		
		{
		{497, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\497\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\497\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{498, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\498\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\498\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{499, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\499\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\499\\Src\\", {BLOCK10, EMPTY},
		},
		},

		{
		{500, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\500\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\500\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{501, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\501\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\501\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{502, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\502\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\502\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{503, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\503\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\503\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{504, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\504\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\504\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{505, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\505\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\505\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{506, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\506\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\506\\Src\\", {BLOCK11, EMPTY},
		},
		},
		{
		{507, EFileAttributes}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\507\\Src\\\x65B0(A)\x6587\\\x65B0\x65B0\x65B0(ABGH)\x65B0\x65B0\x65B0\\", 
		(TText*)L"?:\\T_FCSC\\507\\Src\\", {BLOCK20, EMPTY},
		},
		},
		
//Cases for RFs::Entry() with Mixed characters:		
		{
		{508, ERFsEntry}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\508\\Src\\\x65B0\x6587(AB)\\", 
		(TText*)L"?:\\T_FCSC\\508\\Src\\", {BLOCK21, EMPTY},
		},
		},
		
//Cases for RFs::GetDir() with Mixed characters:	
		{
		{509, EGetDir}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\509\\Src\\\x65B0\x6587(AB)\\\x65B0\x4EF6(ABCDEF)\x4EF6\x6587\\", 
		(TText*)L"?:\\T_FCSC\\509\\Src\\", {BLOCK21, EMPTY},
		},
		}, 
		
//Cases for RFs::MkDir() with Mixed characters:		
		{
		{510, EMkDir}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\510\\Src\\\x65B0\x6587(AB)\\", 
		(TText*)L"?:\\T_FCSC\\510\\Src\\", {EMPTY, EMPTY},
		},
		},
		 
//Cases for RFs::MkDirAll() with Mixed characters:				
		{
		{511, EMkDirAll}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\511\\Src\\\x65B0\x6587(AB)\\\x65B0\x4EF6(AB)\x4EF6\\", 
		(TText*)L"?:\\T_FCSC\\511\\Src\\", {EMPTY, EMPTY},
		},
		}, 
		
		//Cases for RDir::Read() with only alphabetic characters:
		{
		{512, EReadDir , 0, KErrEof, KErrEof, KErrEof}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\512\\Src\\TESTDIR\\", 
		(TText*)L"?:\\T_FCSC\\512\\Src\\", {BLOCK12, EMPTY},
		},
		},

//Cases for RDir::Read() with Unicode characters:		
		{
		{513, EReadDir, 0, KErrEof, KErrEof, KErrEof}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\513\\Src\\\x65B0\x6587\\", 
		(TText*)L"?:\\T_FCSC\\513\\Src\\", {BLOCK17, EMPTY},
		},
		},
		
//Cases for RDir::Read() with Mixed characters:		
		{
		{514, EReadDir,0, KErrEof, KErrEof, KErrEof}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\514\\Src\\\x65B0(A)\x6587\\", 
		(TText*)L"?:\\T_FCSC\\514\\Src\\", {BLOCK20, EMPTY},
		},
		},
		
//Cases for RFile::Read() with only alphabetic characters:	
		{
		{515, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\515\\Src\\ABC(DE).TX", 
		(TText*)L"?:\\T_FCSC\\515\\Src\\", {BLOCK01, EMPTY},
		},
		},

		{
		{516, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\516\\Src\\ABC(DE).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\516\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{517, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\517\\Src\\ABC(DE).TXT", 
		(TText*)L"?:\\T_FCSC\\517\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{518, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\518\\Src\\ABCDEF(GH).TX", 
		(TText*)L"?:\\T_FCSC\\518\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{519, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\519\\Src\\ABCDEF(GH).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\519\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{520, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\520\\Src\\ABCDEF(GH).TXT", 
		(TText*)L"?:\\T_FCSC\\520\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{521, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\521\\Src\\ABC(DEF).TX", 
		(TText*)L"?:\\T_FCSC\\521\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{522, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\522\\Src\\ABC(DEF).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\522\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{523, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\523\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\523\\Src\\", {BLOCK01, EMPTY},
		},
		},
	
//Cases for RFile::Read() with Unicode characters:		
		{
		{524, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\524\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\524\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{525, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\525\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\525\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{526, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\526\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\526\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{527, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\527\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\527\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{528, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\528\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\528\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{529, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\529\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\529\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{530, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\530\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\530\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{531, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\531\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\531\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{532, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\532\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\532\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{533, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\533\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\533\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{534, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\534\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\534\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{535, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\535\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\535\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{536, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\536\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\536\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{537, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\537\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\537\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{538, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\538\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\538\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{539, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\539\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\539\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{540, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\540\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\540\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{541, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\541\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\541\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{542, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\542\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\542\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{543, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\543\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\543\\Src\\", {BLOCK06, EMPTY},
		},
		},
					
//Cases for RFile::Read() with Mixed characters:
		{
		{544, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\544\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\544\\Src\\", {BLOCK07, EMPTY},
		},
		},
			
		{
		{545, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\545\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\545\\Src\\", {BLOCK07, EMPTY},
		},
		},	
		
		{
		{546,EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\546\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\546\\Src\\", {BLOCK07, EMPTY},
		},
		},
		
		{
		{547,EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\547\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\547\\Src\\", {BLOCK07, EMPTY},
		},
		}, 
		
		{
		{548, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\548\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\548\\Src\\", {BLOCK07, EMPTY},
		},
		},
		
		{
		{549, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\549\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\549\\Src\\", {BLOCK08, EMPTY},
		},
		},  
		
		{
		{550, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\550\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\550\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{551, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\551\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\551\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{552, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\552\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\552\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{553, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\553\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\553\\Src\\", {BLOCK08, EMPTY},
		},
		},
			
		{
		{554, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\554\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\554\\Src\\", {BLOCK09, EMPTY},
		},
		},	
		
		{
		{555, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\555\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\555\\Src\\", {BLOCK09, EMPTY},
		},
		},
		
		{
		{556, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\556\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\556\\Src\\", {BLOCK09, EMPTY},
		},
		}, 
		
		{
		{557, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\557\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\557\\Src\\", {BLOCK09, EMPTY},
		},
		},
		
		{
		{558, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\558\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\558\\Src\\", {BLOCK09, EMPTY},
		},
		},  
		
		{
		{559, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\559\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\559\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{560, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\560\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\560\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{561, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\561\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\561\\Src\\", {BLOCK10, EMPTY},
		},
		},

		{
		{562, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\562\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\562\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{563, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\563\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\563\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{564, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\564\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\564\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{565, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\565\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\565\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{566, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\566\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\566\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{567, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\567\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\567\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{568, EReadFromFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\568\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\568\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
//Cases for RFile::Write() with only alphabetic characters:		
		{
		{569, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\569\\Src\\ABC(DE).TX", 
		(TText*)L"?:\\T_FCSC\\569\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{570, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\570\\Src\\ABC(DE).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\570\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{571, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\571\\Src\\ABC(DE).TXT", 
		(TText*)L"?:\\T_FCSC\\571\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{572, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\572\\Src\\ABCDEF(GH).TX", 
		(TText*)L"?:\\T_FCSC\\572\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{573, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\573\\Src\\ABCDEF(GH).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\573\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{574, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\574\\Src\\ABCDEF(GH).TXT", 
		(TText*)L"?:\\T_FCSC\\574\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{575, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\575\\Src\\ABC(DEF).TX", 
		(TText*)L"?:\\T_FCSC\\575\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{576, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\576\\Src\\ABC(DEF).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\576\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
		{
		{577, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\577\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\577\\Src\\", {BLOCK01, EMPTY},
		},
		},
		
//Cases for RFile::Write() with Unicode characters:
		{
		{578, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\578\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\578\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{579, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\579\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\579\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{580, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\580\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\580\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{581, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\581\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\581\\Src\\", {BLOCK02, EMPTY},
		},
		},
		
		{
		{582, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\582\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\582\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{583, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\583\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\583\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{584, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\584\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\584\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{585, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\585\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\585\\Src\\", {BLOCK03, EMPTY},
		},
		},
		
		{
		{586, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\586\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\586\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{587, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\587\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\587\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{588, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\588\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\588\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{589, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\589\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\589\\Src\\", {BLOCK04, EMPTY},
		},
		},
		
		{
		{590, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\590\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\590\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{591, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\591\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\591\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{592, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\592\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\592\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{593, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\593\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\593\\Src\\", {BLOCK05, EMPTY},
		},
		},
		
		{
		{594, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\594\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\594\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{595, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\595\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\595\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{596, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\596\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\596\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
		{
		{597, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\597\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\597\\Src\\", {BLOCK06, EMPTY},
		},
		},
		
//Cases for RFile::Write() with Mixed characters:
		{
		{598, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\598\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\598\\Src\\", {BLOCK07, EMPTY},
		},
		},
			
		{
		{599, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\599\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\599\\Src\\", {BLOCK07, EMPTY},
		},
		},	
		
		{
		{600,EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\600\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\600\\Src\\", {BLOCK07, EMPTY},
		},
		},
		
		{
		{601,EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\601\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\601\\Src\\", {BLOCK07, EMPTY},
		},
		}, 
		
		{
		{602, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\602\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\602\\Src\\", {BLOCK07, EMPTY},
		},
		},
		
		{
		{603, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\603\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\603\\Src\\", {BLOCK08, EMPTY},
		},
		},  
		
		{
		{604, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\604\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\604\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{605, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\605\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\605\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{606, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\606\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\606\\Src\\", {BLOCK08, EMPTY},
		},
		},
		
		{
		{607, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\607\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\607\\Src\\", {BLOCK08, EMPTY},
		},
		},
			
		{
		{608, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\608\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\608\\Src\\", {BLOCK09, EMPTY},
		},
		},	
		
		{
		{609, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\609\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\609\\Src\\", {BLOCK09, EMPTY},
		},
		},
		
		{
		{610, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\610\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\610\\Src\\", {BLOCK09, EMPTY},
		},
		}, 
		
		{
		{611, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\611\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\611\\Src\\", {BLOCK09, EMPTY},
		},
		},
		
		{
		{612, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\612\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\612\\Src\\", {BLOCK09, EMPTY},
		},
		},  
		
		{
		{613, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\613\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\613\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{614, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\614\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\614\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{615, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\615\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\615\\Src\\", {BLOCK10, EMPTY},
		},
		},

		{
		{616, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\616\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\616\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{617, EWriteToFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\617\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\617\\Src\\", {BLOCK10, EMPTY},
		},
		},
		
		{
		{618, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\618\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\618\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{619, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\619\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\619\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{620, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\620\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\620\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{621, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\621\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\621\\Src\\", {BLOCK11, EMPTY},
		},
		},
		
		{
		{622, EWriteToFile, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\622\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\622\\Src\\", {BLOCK11, EMPTY},
		},
		},		
		//	End unary API test cases 		
		{{0}}
	 	
	}; 

static TTestCaseBinaryBasic gBasicBinaryTestCases[] =
	{

//*********************only with alphabetic characters**************************************				

//Cases for RFs::GetShortName() with alphabetic characters-same with or without DLL:	
		{
		{623, EGetShortName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\623\\Src\\ABC(DE).TX", 
		(TText*)L"?:\\T_FCSC\\623\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\623\\Src\\ABC(DE).TX", 
		}
		},

		{
		{624, EGetShortName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\624\\Src\\ABC(DE).TXT", 
		(TText*)L"?:\\T_FCSC\\624\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\624\\Src\\ABC(DE).TXT", 
		}
		},	

		{
		{625, EGetShortName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\625\\Src\\ABC(DE).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\625\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\625\\Src\\ABC(DE~1.TXT", 
		}
		},

		{
		{626, EGetShortName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\626\\Src\\ABCDEF(GH).TX", 
		(TText*)L"?:\\T_FCSC\\626\\Src\\", {BLOCK01, EMPTY},
		},

		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\626\\Src\\ABCDEF~1.TX", 
		}
		}, 

		{
		{627, EGetShortName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\627\\Src\\ABCDEF(GH).TXT", 
		(TText*)L"?:\\T_FCSC\\627\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\627\\Src\\ABCDEF~2.TXT", 
		}
		},
		
		{
		{628, EGetShortName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\628\\Src\\ABCDEF(GH).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\628\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\628\\Src\\ABCDEF~1.TXT", 
		}
		},  
		
		{
		{629, EGetShortName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\629\\Src\\ABC(DEF).TX", 
		(TText*)L"?:\\T_FCSC\\629\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\629\\Src\\ABC(DEF).TX", 
		}
		},
		
		{
		{630, EGetShortName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\630\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\630\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\630\\Src\\ABC(DEF).TXT", 
		}
		},
		
		{
		{631, EGetShortName}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\631\\Src\\ABC(DEF).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\631\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\631\\Src\\ABC(DE~2.TXT", 
		}
		}, 
		
//Cases for RFs::Rename() with alphabetic characters:	
		{
		{632, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\632\\Src\\ABC(DE).TX", 
		(TText*)L"?:\\T_FCSC\\632\\Src\\", {{0,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\632\\Src\\FS_RENAMED1.TXTTXT", 
		}
		},
		
		{
		{633, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\633\\Src\\ABC(DE).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\633\\Src\\", {{1,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\633\\Src\\FS_RENAMED2.TX", 
		}
		},
		
		{
		{634, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\634\\Src\\ABC(DE).TXT", 
		(TText*)L"?:\\T_FCSC\\634\\Src\\", {{2,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\634\\Src\\FS_RENAMED3.REN", 
		}
		},
		
		{
		{635, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\635\\Src\\ABCDEF(GH).TX", 
		(TText*)L"?:\\T_FCSC\\635\\Src\\", {{3,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\635\\Src\\FS_RENAMED4.DAT", 
		}
		},
		
		{
		{636, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\636\\Src\\ABCDEF(GH).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\636\\Src\\", {{4,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\636\\Src\\FS_RENAMED5.TXT", 
		}
		},
		
		{
		{637, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\637\\Src\\ABCDEF(GH).TXT", 
		(TText*)L"?:\\T_FCSC\\637\\Src\\", {{5,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\637\\Src\\FS_RENAMED6.TXTTXT", 
		}
		},
		
		{
		{638, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\638\\Src\\ABC(DEF).TX", 
		(TText*)L"?:\\T_FCSC\\638\\Src\\", {{6,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\638\\Src\\FS_RENAMED7.TX", 
		}
		},
		
		{
		{639, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\639\\Src\\ABC(DEF).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\639\\Src\\", {{7,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\639\\Src\\FS_RENAMED8.RENDAT", 
		}
		},
		
		{
		{640, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\640\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\640\\Src\\", {{8,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\640\\Src\\FS_RENAMED8.TX", 
		}
		}, 
		
		{
		{641, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\641\\Src\\TESTALPHA.TXTTXT", 
		(TText*)L"?:\\T_FCSC\\641\\Src\\", {{67,71,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\641\\Src\\XYZ\\RENAMED1.TXT", 
		}
		},

		{
		{642, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\642\\Src\\TESTDIR_FS\\DIR1\\", 
		(TText*)L"?:\\T_FCSC\\642\\Src\\", {BLOCK14, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\642\\Src\\TESTDIR_FS\\DIR_RENAMED\\", 
		}
		}, 
		
//Cases for RFile::Rename() with alphabetic characters:
		{
		{643,ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\643\\Src\\ABC(DE).TX", 
		(TText*)L"?:\\T_FCSC\\643\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\643\\Src\\RENAMED1.TXTTXT", 
		},
		},
		
		{
		{644, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\644\\Src\\ABC(DE).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\644\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\644\\Src\\RENAMED2.TX", 
		},
		},
		
		{
		{645, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\645\\Src\\ABC(DE).TXT", 
		(TText*)L"?:\\T_FCSC\\645\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\645\\Src\\RENAMED3.DAT", 
		},
		},
		
		{
		{646, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\646\\Src\\ABCDEF(GH).TX", 
		(TText*)L"?:\\T_FCSC\\646\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\646\\Src\\RENAMED4.REN", 
		},
		},
		
		{
		{647, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\647\\Src\\ABCDEF(GH).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\647\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\647\\Src\\RENAMED5.TXTTXT", 
		},
		},
		
		{
		{648, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\648\\Src\\ABCDEF(GH).TXT", 
		(TText*)L"?:\\T_FCSC\\648\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\648\\Src\\RENAMED6.TX", 
		},
		},
		
		{
		{649, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\649\\Src\\ABC(DEF).TX", 
		(TText*)L"?:\\T_FCSC\\649\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\649\\Src\\RENAMED7.TX", 
		},
		},
		
		{
		{650, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\650\\Src\\ABC(DEF).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\650\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\650\\Src\\RENAMED8.TXTTXT", 
		},
		},
		
		{
		{651, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\651\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\651\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\651\\Src\\RENAMED9.TXT", 
		},
		},  
				
		{
		{652, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\652\\Src\\TESTRENAME.DAT", 
		(TText*)L"?:\\T_FCSC\\652\\Src\\", {{71,72,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\652\\Src\\XYZ\\RENAMED1.TXTTXT", 
		}
		},
		
		
//Cases for RFs::Replace() with alphabetic characters:		
		{
		{653,ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\653\\Src\\ABC(DE).TX", 
		(TText*)L"?:\\T_FCSC\\653\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\653\\Src\\FS_RENAMED1.TXTTXT", 
		},
		},
		
		{
		{654, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\654\\Src\\ABC(DE).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\654\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\654\\Src\\FS_RENAMED2.TX", 
		},
		},
		
		{
		{655, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\655\\Src\\ABC(DE).TXT", 
		(TText*)L"?:\\T_FCSC\\655\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\655\\Src\\FS_RENAMED3.DAT", 
		},
		},
		
		{
		{656, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\656\\Src\\ABCDEF(GH).TX", 
		(TText*)L"?:\\T_FCSC\\656\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\656\\Src\\FS_RENAMED4.REN", 
		},
		},
		
		{
		{657, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\657\\Src\\ABCDEF(GH).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\657\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\657\\Src\\FS_RENAMED5.TXTTXT", 
		},
		},
		
		{
		{658, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\658\\Src\\ABCDEF(GH).TXT", 
		(TText*)L"?:\\T_FCSC\\658\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\658\\Src\\FS_RENAMED6.TX", 
		},
		},
		
		{
		{659, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\659\\Src\\ABC(DEF).TX", 
		(TText*)L"?:\\T_FCSC\\659\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\659\\Src\\FS_RENAMED7.TX", 
		},
		},
		
		{
		{660, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\660\\Src\\ABC(DEF).TXTTXT", 
		(TText*)L"?:\\T_FCSC\\660\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\660\\Src\\FS_RENAMED8.TXTTXT", 
		},
		},
		
		{
		{661, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\661\\Src\\ABC(DEF).TXT", 
		(TText*)L"?:\\T_FCSC\\661\\Src\\", {BLOCK01, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\661\\Src\\FS_RENAMED9.TXT", 
		},
		},  
				
		{
		{662, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\662\\Src\\TESTRENAME.DAT", 
		(TText*)L"?:\\T_FCSC\\662\\Src\\", {{71,72,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\662\\Src\\XYZ\\RENAMED1.TXTTXT", 
		}
		},
		

// *********************only with Unicode characters****************************************
		
//Cases for RFs::GetShortName() with Unicode characters: With DLL:			
		{
		{663, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\663\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\663\\Src\\", {BLOCK02, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\663\\Src\\\x65B0\x65B0.\x65B0", 
		}
		},
			
		{
		{664, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\664\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\664\\Src\\", {BLOCK02, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\664\\Src\\\x65B0\x6587~1.\x65B0", 
		}
		},	
		
		{
		{665, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\665\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\665\\Src\\", {BLOCK02, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\665\\Src\\\x65B0\x65B0~1.\x65B0", 
		}
		},
		
		{
		{666, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\666\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\666\\Src\\", {BLOCK02, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\666\\Src\\\x65B0\x65B0~2.\x65B0", 
		}
		}, 
		
		{
		{667, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\667\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\667\\Src\\", {BLOCK03, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\667\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		}
		},
		
		{
		{668, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\668\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\668\\Src\\", {BLOCK03, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\668\\Src\\\x65B0\x65B0\x65B0~1.\x65B0", 
		}
		},  
		
		{
		{669, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\669\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\669\\Src\\", {BLOCK03, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\669\\Src\\\x65B0\x65B0\x65B0~2.\x65B0", 
		}
		},
		
		{
		{670, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\670\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\670\\Src\\", {BLOCK03, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\670\\Src\\\x65B0\x4EF6\x65B0~1.\x65B0", 
		}
		},
		
		{
		{671, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\671\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\671\\Src\\", {BLOCK04, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\671\\Src\\\x65B0\x65B0\x65B0~1.\x65B0", 
		}
		},
		
		{
		{672, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\672\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\672\\Src\\", {BLOCK04, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\672\\Src\\\x65B0\x4EF6\x65B0~1.\x65B0", 
		}
		},
			
		{
		{673, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\673\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\673\\Src\\", {BLOCK04, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\673\\Src\\\x65B0\x6587\x65B0~1.\x65B0", 
		}
		},	
		
		{
		{674, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\674\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\674\\Src\\", {BLOCK04, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\674\\Src\\\x65B0\x6587\x6587~1.\x65B0", 
		}
		},
		
		{
		{675, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\675\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\675\\Src\\", {BLOCK05, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\675\\Src\\\x65B0\x4EF6\x65B0~1.\x4EF6", 
		}
		}, 
		
		{
		{676, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\676\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\676\\Src\\", {BLOCK05, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\676\\Src\\\x4EF6\x4EF6\x65B0~1.\x65B0", 
		}
		},
		
		{
		{677, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\677\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\677\\Src\\", {BLOCK05, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\677\\Src\\\x65B0\x65B0\x65B0~1.\x65B0", 
		}
		},  
		
		{
		{678, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\678\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\678\\Src\\", {BLOCK05, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\678\\Src\\\x65B0\x65B0\x65B0~2.\x65B0", 
		}
		},
		
		{
		{679, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\679\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\679\\Src\\", {BLOCK06, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\679\\Src\\\x6587\x6587\x6587~1.\x65B0", 
		}
		},
		
		{
		{680, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\680\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\680\\Src\\", {BLOCK06, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\680\\Src\\\x4EF6\x4EF6\x65B0~1.\x65B0", 
		}
		},
		
		{
		{681, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\681\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\681\\Src\\", {BLOCK06, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\681\\Src\\\x65B0\x65B0\x65B0~1.\x65B0", 
		}
		},
		
		{
		{682, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\682\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\682\\Src\\", {BLOCK06, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\682\\Src\\\x65B0\x65B0\x4EF6~1.\x65B0", 
		}
		},
	
//Cases for RFs::GetShortName() with Unicode characters: Without DLL:
		{
		{683, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\683\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\683\\Src\\", {BLOCK02, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\683\\Src\\__._", 
		}
		},
			
		{
		{684, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\684\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\684\\Src\\", {BLOCK02, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\684\\Src\\__.__", 
		}
		},	
		
		{
		{685, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\685\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\685\\Src\\", {BLOCK02, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\685\\Src\\__.___", 
		}
		}, 
		
		{
		{686, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\686\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\686\\Src\\", {BLOCK02, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\686\\Src\\__~1.___", 
		}
		}, 
		
		{
		{687, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\687\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\687\\Src\\", {BLOCK03, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\687\\Src\\____._", 
		}
		}, 
	
		
		{
		{688, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\688\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\688\\Src\\", {BLOCK03, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\688\\Src\\____.__", 
		}
		},  
		
		{
		{689, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\689\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\689\\Src\\", {BLOCK03, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\689\\Src\\____.___", 
		}
		},
		
		{
		{690, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\690\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\690\\Src\\", {BLOCK03, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\690\\Src\\____~1.___", 
		}
		},
		
		{
		{691, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\691\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\691\\Src\\", {BLOCK04, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\691\\Src\\______._", 
		}
		},
		
		{
		{692, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\692\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\692\\Src\\", {BLOCK04, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\692\\Src\\_____.__", 
		}
		},
			
		{
		{693, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\693\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\693\\Src\\", {BLOCK04, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\693\\Src\\_____.___", 
		}
		},	
		
		{
		{694, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\694\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\694\\Src\\", {BLOCK04, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\694\\Src\\_____~1.___", 
		}
		},	
		
		{
		{695, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\695\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\695\\Src\\", {BLOCK05, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\695\\Src\\________._", 
		}
		}, 
		
		{
		{696, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\696\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\696\\Src\\", {BLOCK05, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\696\\Src\\________.__", 
		}
		},
		
		{
		{697, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\697\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\697\\Src\\", {BLOCK05, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\697\\Src\\_______.___", 
		}
		},  
		
		{
		{698, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\698\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\698\\Src\\", {BLOCK05, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\698\\Src\\______~1.___", 
		}
		},
		
		{
		{699, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\699\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\699\\Src\\", {BLOCK06, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\699\\Src\\______~1._", 
		}
		},
		
		{
		{700, EGetShortNameWithoutDLL}, 
		{&gDriveToTest,(TText*)L"?:\\T_FCSC\\700\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\700\\Src\\", {BLOCK06, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\700\\Src\\______~1.__", 
		}
		},
		
		
		{
		{701, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\701\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\701\\Src\\", {BLOCK06, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\701\\Src\\______~1.___", 
		}
		},
		
		{
		{702, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\702\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\702\\Src\\", {BLOCK06, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\702\\Src\\______~2.___", 
		}
		}, 
		   	
//Cases for RFs::Rename() with Unicode characters:
		{
		{703, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\703\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\703\\Src\\", {{9,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\703\\Src\\\x6587\x6587\x65B0\x6587\x6587\x65B0.\x6587x6587x6587", 
		}
		},
			
		{
		{704, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\704\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\704\\Src\\", {{10,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\704\\Src\\\x65B0\x4EF6\x65B0.\x65B0\x6587", 
		}
		},	
		
		{
		{705, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\705\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\705\\Src\\", {{11,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\705\\Src\\\x4EF6\x4EF6\x4EF6.\x65B0", 
		}
		},
		
		{
		{706, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\706\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\706\\Src\\", {{12,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\706\\Src\\\x6587\x6587.\x6587", 
		}
		}, 
		
		{
		{707, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\707\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\707\\Src\\", {{13,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\707\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		}
		},
		
		{
		{708, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\708\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\708\\Src\\", {{14,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\708\\Src\\\x6587\x6587.\x65B0", 
		}
		},  
		
		{
		{709, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\709\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\709\\Src\\", {{15,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\709\\Src\\\x4EF6\x6587.\x4EF6\x65B0\x4EF6", 
		}
		},
		
		{
		{710, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\710\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\710\\Src\\", {{16,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\710\\Src\\\x4EF6\x4EF6\x4EF6\x4EF6\x4EF6\x4EF6.\x4EF6\x4EF6\x4EF6\x4EF6\x4EF6\x4EF6", 
		}
		},
		
		{
		{711, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\711\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\711\\Src\\", {{17,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\711\\Src\\\x6587\x4EF6.\x4EF6", 
		}
		},
		
		{
		{712, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\712\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\712\\Src\\", {{18,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\712\\Src\\\x4EF6\x65B0\x6587\x4EF6\x4EF6.\x65B0\x6587\x4EF6", 
		}
		},
			
		{
		{713, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\713\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\713\\Src\\", {{19,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\713\\Src\\\x65B0\x4EF6\x65B0\x65B0\x4EF6\x65B0.\x4EF6", 
		}
		},	
		
		{
		{714, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\714\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\714\\Src\\", {{20,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\714\\Src\\\x4EF6\x6587.\x65B0\x6587\x6587\x4EF6\x6587\x65B0", 
		}
		},
		
		{
		{715, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\715\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\715\\Src\\", {{21,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\715\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6\x4EF6\x65B0\x4EF6", 
		}
		}, 
		
		{
		{716, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\716\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\716\\Src\\", {{22,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\716\\Src\\\x4EF6\x65B0.\x65B0\x65B0\x6587", 
		}
		},
		
		{
		{717, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\717\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\717\\Src\\", {{23,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\717\\Src\\\x65B0\x4EF6.\x65B0", 
		}
		},  
		
		{
		{718, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\718\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\718\\Src\\", {{24,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\718\\Src\\\x4EF6\x65B0\x4EF6\x4EF6.\x4EF6\x65B0\x4EF6\x4EF6", 
		}
		},
		
		{
		{719, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\719\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\719\\Src\\", {{25,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\719\\Src\\\x4EF6\x4EF6\x6587\x6587\x4EF6\x4EF6\x65B0.\x4EF6\x4EF6\x4EF6\x65B0\x4EF6\x4EF6", 
		}
		},
		
		{
		{720, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\720\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\720\\Src\\", {{26,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\720\\Src\\\x65B0\x65B0.\x65B0\x6587", 
		}
		},
		
		{
		{721, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\721\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\721\\Src\\", {{27,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\721\\Src\\\x6587\x6587\x6587.\x6587\x6587\x6587", 
		}
		},
		
		{
		{722, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\722\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\722\\Src\\", {{28,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\722\\Src\\\x4EF6\x4EF6\x4EF6(\x65B0\x65B0)\x65B0\x4EF6\x65B0\x65B0.\x65B0", 
		}
		},

		
		{
		{723, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\723\\Src\\\x65B0\x6587\x4EF6\x4EF6\x6587\x65B0.\x65B0\x4EF6", 
		(TText*)L"?:\\T_FCSC\\723\\Src\\", {{73,74,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\723\\Src\\\x65B0\x65B0\x65B0\\\x6587\x65B0(\x4EF6).\x4EF6", 
		}
		},
		
		{
		{724, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\724\\Src\\\x65B0\x6587\x4EF6\\\x65B0\x4EF6\x65B0\x4EF6\\", 
		(TText*)L"?:\\T_FCSC\\724\\Src\\", {BLOCK19, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\724\\Src\\\x65B0\x6587\x4EF6\\\x65B0\x65B0\x65B0\x65B0\\", 
		}
		},

//Cases for RFile::Rename() with Unicode characters:
		{
		{725, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\725\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\725\\Src\\", {{9,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\725\\Src\\\x6587(\x6587\x65B0)\x6587\x6587.\x6587x6587x6587", 
		}
		},
			
		{
		{726, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\726\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\726\\Src\\", {{10,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\726\\Src\\\x65B0\x4EF6(\x65B0).\x65B0\x6587", 
		}
		},	
		
		{
		{727, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\727\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\727\\Src\\", {{11,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\727\\Src\\\x4EF6\x4EF6\x4EF6(.\x65B0)", 
		}
		},
		
		{
		{728, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\728\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\728\\Src\\", {{12,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\728\\Src\\\x6587\x6587.\x6587", 
		}
		}, 
		
		{
		{729, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\729\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\729\\Src\\", {{13,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\729\\Src\\\x65B0\x65B0.(\x65B0\x65B0\x65B0", 
		}
		},
		
		{
		{730, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\730\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\730\\Src\\", {{14,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\730\\Src\\\x6587\x6587.\x65B0", 
		}
		},  
		
		{
		{731, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\731\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\731\\Src\\", {{15,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\731\\Src\\\x4EF6\x6587.\x4EF6(\x65B0\x4EF6)", 
		}
		},
		
		{
		{732, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\732\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\732\\Src\\", {{16,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\732\\Src\\\x6587\x4EF6.\x6587\x4EF6", 
		}
		},
		
		{
		{733, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\733\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\733\\Src\\", {{17,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\733\\Src\\\x6587\x4EF6.\x4EF6", 
		}
		},
		
		{
		{734, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\734\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\734\\Src\\", {{18,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\734\\Src\\\x4EF6(\x65B0\x6587)\x4EF6\x4EF6.\x65B0\x6587\x4EF6", 
		}
		},
			
		{
		{735, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\735\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\735\\Src\\", {{19,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\735\\Src\\\x65B0\x4EF6\x65B0\x65B0.\x4EF6", 
		}
		},	
		
		{
		{736, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\736\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\736\\Src\\", {{20,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\736\\Src\\\x4EF6\x6587.\x65B0\x6587\x6587\x4EF6\x6587\x65B0", 
		}
		},
		
		{
		{737, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\737\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\737\\Src\\", {{21,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\737\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6\x4EF6\x65B0\x4EF6", 
		}
		}, 
		
		{
		{738, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\738\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\738\\Src\\", {{22,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\738\\Src\\\x4EF6\x65B0.\x65B0\x65B0\x6587", 
		}
		},
		
		{
		{739, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\739\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\739\\Src\\", {{23,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\739\\Src\\\x65B0\x4EF6.\x65B0", 
		}
		},  
		
		{
		{740, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\740\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\740\\Src\\", {{24,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\740\\Src\\\x4EF6\x65B0\x4EF6\x4EF6.\x4EF6\x65B0\x4EF6\x4EF6", 
		}
		},
		
		{
		{741, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\741\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\741\\Src\\", {{25,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\741\\Src\\\x4EF6\x4EF6\x6587\x6587\x4EF6\x4EF6\x65B0.\x4EF6\x4EF6\x4EF6\x65B0\x4EF6\x4EF6", 
		}
		},
		
		{
		{742, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\742\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\742\\Src\\", {{26,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\742\\Src\\\x65B0\x65B0.\x65B0\x6587", 
		}
		},
		
		{
		{743, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\743\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\743\\Src\\", {{27,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\743\\Src\\\x6587\x6587\x6587.\x6587\x6587\x6587", 
		}
		},
		
		{
		{744, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\744\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\744\\Src\\", {{28,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\744\\Src\\\x4EF6\x4EF6\x4EF6(\x65B0\x65B0)\x65B0\x4EF6\x65B0\x65B0.\x65B0", 
		}
		},
	
//Cases for RFs::Replace() with Unicode characters:
		{
		{745, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\745\\Src\\\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\745\\Src\\", {{9,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\745\\Src\\\x6587(\x6587\x65B0)\x6587\x6587.\x6587x6587x6587", 
		}
		},
			
		{
		{746, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\746\\Src\\\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\746\\Src\\", {{10,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\746\\Src\\\x65B0\x4EF6(\x65B0).\x65B0\x6587", 
		}
		},	
		
		{
		{747, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\747\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\747\\Src\\", {{11,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\747\\Src\\\x4EF6\x4EF6\x4EF6(.\x65B0)", 
		}
		},
		
		{
		{748, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\748\\Src\\\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\748\\Src\\", {{12,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\748\\Src\\\x6587\x6587.\x6587", 
		}
		}, 
		
		{
		{749, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\749\\Src\\\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\749\\Src\\", {{13,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\749\\Src\\\x65B0\x65B0.(\x65B0\x65B0\x65B0", 
		}
		},
		
		{
		{750, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\750\\Src\\\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\750\\Src\\", {{14,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\750\\Src\\\x6587\x6587.\x65B0", 
		}
		},  
		
		{
		{751, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\751\\Src\\\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\751\\Src\\", {{15,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\751\\Src\\\x4EF6\x6587.\x4EF6(\x65B0\x4EF6)", 
		}
		},
		
		{
		{752, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\752\\Src\\\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\752\\Src\\", {{16,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\752\\Src\\\x6587\x4EF6.\x6587\x4EF6", 
		}
		},
		
		{
		{753,ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\753\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		(TText*)L"?:\\T_FCSC\\753\\Src\\", {{17,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\753\\Src\\\x6587\x4EF6.\x4EF6", 
		}
		},
		
		{
		{754, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\754\\Src\\\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\754\\Src\\", {{18,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\754\\Src\\\x4EF6(\x65B0\x6587)\x4EF6\x4EF6.\x65B0\x6587\x4EF6", 
		}
		},
			
		{
		{755, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\755\\Src\\\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\755\\Src\\", {{19,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\755\\Src\\\x65B0\x4EF6\x65B0\x65B0.\x4EF6", 
		}
		},	
		
		{
		{756, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\756\\Src\\\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\756\\Src\\", {{20,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\756\\Src\\\x4EF6\x6587.\x65B0\x6587\x6587\x4EF6\x6587\x65B0", 
		}
		},
		
		{
		{757, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\757\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\757\\Src\\", {{21,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\757\\Src\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6\x4EF6\x65B0\x4EF6", 
		}
		}, 
		
		{
		{758, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\758\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\758\\Src\\", {{22,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\758\\Src\\\x4EF6\x65B0.\x65B0\x65B0\x6587", 
		}
		},
		
		{
		{759, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\759\\Src\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0", 
		(TText*)L"?:\\T_FCSC\\759\\Src\\", {{23,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\759\\Src\\\x65B0\x4EF6.\x65B0", 
		}
		},  
		
		{
		{760, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\760\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\760\\Src\\", {{24,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\760\\Src\\\x4EF6\x65B0\x4EF6\x4EF6.\x4EF6\x65B0\x4EF6\x4EF6", 
		}
		},
		
		{
		{761, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\761\\Src\\\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0", 
		(TText*)L"?:\\T_FCSC\\761\\Src\\", {{25,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\761\\Src\\\x4EF6\x4EF6\x6587\x6587\x4EF6\x4EF6\x65B0.\x4EF6\x4EF6\x4EF6\x65B0\x4EF6\x4EF6", 
		}
		},
		
		{
		{762, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\762\\Src\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\762\\Src\\", {{26,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\762\\Src\\\x65B0\x65B0.\x65B0\x6587", 
		}
		},
		
		{
		{763, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\763\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\763\\Src\\", {{27,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\763\\Src\\\x6587\x6587\x6587.\x6587\x6587\x6587", 
		}
		},
		
		{
		{764, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\764\\Src\\\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\764\\Src\\", {{28,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\764\\Src\\\x4EF6\x4EF6\x4EF6(\x65B0\x65B0)\x65B0\x4EF6\x65B0\x65B0.\x65B0", 
		}
		},

		{
		{765, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\765\\Src\\\x65B0\x6587\x4EF6\\\x65B0\x4EF6\x65B0\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6", 
		(TText*)L"?:\\T_FCSC\\765\\Src\\", {BLOCK19, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\765\\Src\\\x65B0\x6587\x4EF6\\\x65B0\x4EF6\x65B0\\\x65B0\x4EF6\x65B0.\x4EF6\x6587\x65B0", 
		}
		}, 
 
		
//*********************Mixed(alpha and unicode characters**********************************		
	
//Cases for RFs::GetShortName() with Mixed characters: With DLL:		
		{
		{766, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\766\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\766\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\766\\Src\\\x65B0(A).\x65B0", 
		}
		},
			
		{
		{767, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\767\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\767\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\767\\Src\\\x65B0(A).A\x65B0", 
		}
		},	
		
		{
		{768, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\768\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\768\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\768\\Src\\\x65B0(A)~1.A\x65B0", 
		}
		},
		
		{
		{769, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\769\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\769\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\769\\Src\\\x65B0(A)~1.AB", 
		}
		}, 
		
		{
		{770, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\770\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\770\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\770\\Src\\\x65B0(A)~2.AB", 
		}
		},
		
		{
		{771, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\771\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\771\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\771\\Src\\\x65B0\x65B0(AB).\x65B0", 
		}
		},  
		
		{
		{772, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\772\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\772\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\772\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		}
		},
		
		{
		{773, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\773\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\773\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\773\\Src\\\x65B0(AB)~1.A\x65B0", 
		}
		},
		
		{
		{774, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\774\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\774\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\774\\Src\\\x65B0(\x65B0)~1.AB", 
		}
		},
		
		{
		{775, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\775\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\775\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\775\\Src\\\x65B0\x65B0(C~1.AB", 
		}
		},
			
		{
		{776, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\776\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\776\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\776\\Src\\\x65B0\x65B0\x65B0~1.\x65B0", 
		}
		},	
		
		{
		{777, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\777\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\777\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\777\\Src\\\x65B0\x65B0\x65B0~1.A\x65B0", 
		}
		},
		
		{
		{778, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\778\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\778\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\778\\Src\\AB\x65B0\x6587~1.A\x65B0", 
		}
		}, 
		
		{
		{779, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\779\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\779\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\779\\Src\\CD\x65B0\x6587~1.AB", 
		}
		},
		
		{
		{780, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\780\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\780\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\780\\Src\\\x65B0\x6587(~1.AB", 
		}
		},  
		
		{
		{781, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\781\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\781\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\781\\Src\\\x65B0\x65B0\x65B0~1.\x65B0", 
		}
		},
		
		{
		{782, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\782\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\782\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\782\\Src\\(AB)\x65B0~1.A\x65B0", 
		}
		},
		
		{
		{783, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\783\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\783\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\783\\Src\\\x65B0\x65B0\x65B0~1.A\x65B0", 
		}
		},

		{
		{784, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\784\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\784\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\784\\Src\\\x65B0\x65B0\x65B0~1.AB", 
		}
		},
		
		{
		{785, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\785\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\785\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\785\\Src\\\x65B0\x65B0(~1.AB", 
		}
		},
		
		{
		{786, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\786\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\786\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\786\\Src\\\x65B0\x65B0\x65B0~1.\x65B0", 
		}
		},
		
		{
		{787, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\787\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\787\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\787\\Src\\(AB)\x65B0~1.A\x65B0", 
		}
		},
		
		{
		{788, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\788\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\788\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\788\\Src\\\x65B0\x65B0\x65B0~1.A\x65B0", 
		}
		},
		
		{
		{789, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\789\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\789\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\789\\Src\\\x65B0\x65B0\x65B0~1.AB", 
		}
		},
		
		{
		{790, EGetShortNameWithDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\790\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\790\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\790\\Src\\CD\x65B0\x65B0~1.AB", 
		}
		},
		
//Cases for RFs::GetShortName() with Mixed characters: Without DLL:		
		{
		{791, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\791\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\791\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\791\\Src\\_(A)._", 
		}
		},
			
		{
		{792, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\792\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\792\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\792\\Src\\_(A).A_", 
		}
		},	
		
		{
		{793, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\793\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\793\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\793\\Src\\_(A).A__", 
		}
		},
		
		{
		{794, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\794\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\794\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\794\\Src\\_(A)~1.AB_", 
		}
		}, 
		
		{
		{795, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\795\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\795\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\795\\Src\\_(A)~2.AB_", 
		}
		},
		
		{
		{796, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\796\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\796\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\796\\Src\\__(AB)._", 
		}
		},  
		
		{
		{797, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\797\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\797\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\797\\Src\\(AB)__.A_", 
		}
		},
		
		{
		{798, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\798\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\798\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\798\\Src\\_(AB)_.A__", 
		}
		},
		
		{
		{799, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\799\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\799\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\799\\Src\\_(_)AB~1.AB_", 
		}
		},
		
		{
		{800, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\800\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\800\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\800\\Src\\__(CD)~1.AB_", 
		}
		},
			
		{
		{801, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\801\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\801\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\801\\Src\\____(AB)._", 
		}
		},	
		
		{
		{802, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\802\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\802\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\802\\Src\\____(AB).A_", 
		}
		}, 
		
		{
		{803, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\803\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\803\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\803\\Src\\AB____.A__", 
		}
		}, 
		
		{
		{804, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\804\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\804\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\804\\Src\\CD____~1.AB_", 
		}
		},
		
		{
		{805, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\805\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\805\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\805\\Src\\__(__)~1.AB_", 
		}
		},  
		
		{
		{806, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\806\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\806\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\806\\Src\\______~1._", 
		}
		},
		
		{
		{807, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\807\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\807\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\807\\Src\\(AB)__~1.A_", 
		}
		},
		
		{
		{808, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\808\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\808\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\808\\Src\\___(AB~1.A__", 
		}
		},

		{
		{809, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\809\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\809\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\809\\Src\\______~1.AB_", 
		}
		}, 
		
		{
		{810, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\810\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\810\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\810\\Src\\__(__)~1.AB_", 
		}
		},
		
		{
		{811, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\811\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\811\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\811\\Src\\______~1._", 
		}
		},
		
		{
		{812, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\812\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\812\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\812\\Src\\(AB)__~1.A_", 
		}
		},
		
		{
		{813, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\813\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\813\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\813\\Src\\______~1.A__", 
		}
		},
		
		{
		{814, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\814\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\814\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\814\\Src\\___(__~1.AB_", 
		}
		},
		
		{
		{815, EGetShortNameWithoutDLL}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\815\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\815\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\815\\Src\\CD__(_~1.AB_", 
		}
		},		
		
//Cases for RFs::Rename() with Mixed characters:
		{
		{816, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\816\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\816\\Src\\", {{29,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\816\\Src\\FS_RENAME1\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		}
		},
			
		{
		{817, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\817\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\817\\Src\\", {{30,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\817\\Src\\FS_RENAME2\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0", 
		}
		},	
		
		{
		{818, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\818\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\818\\Src\\", {{31,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\818\\Src\\FS_RENAME3\x65B0\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		}
		},
		
		{
		{819, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\819\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\819\\Src\\", {{32,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\819\\Src\\FS_RENAME4\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		}
		}, 
		
		{
		{820, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\820\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\820\\Src\\", {{33,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\820\\Src\\FS_RENAME5\x65B0\x65B0\x65B0\x65B0.\x65B0", 
		}
		},
		
		{
		{821, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\821\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\821\\Src\\", {{34,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\821\\Src\\FS_RENAME6\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		}
		},  
		
		{
		{822, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\822\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\822\\Src\\", {{35,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\822\\Src\\FS_RENAME7\x65B0\x65B0\x65B0.AB\x65B0\x65B0", 
		}
		},
		
		{
		{823, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\823\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\823\\Src\\", {{36,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\823\\Src\\FS_RENAME8\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		}
		},
		
		{
		{824, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\824\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\824\\Src\\", {{37,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\824\\Src\\FS_RENAME9\x65B0\x65B0\x65B0.A\x65B0", 
		}
		},
		
		{
		{825, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\825\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\825\\Src\\", {{38,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\825\\Src\\FSRENAME10\x65B0\x65B0\x65B0.\x65B0", 
		}
		},
			
		{
		{826, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\826\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\826\\Src\\", {{39,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\826\\Src\\FSRENAME11\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		}
		},	
		
		{
		{827, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\827\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\827\\Src\\", {{40,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\827\\Src\\FSRENAME12\x65B0\x65B0.AB\x65B0\x65B0", 
		}
		},
		
		{
		{828, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\828\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\828\\Src\\", {{41,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\828\\Src\\FSRENAME13\x65B0\x65B0.A\x65B0\x6587", 
		}
		}, 
		
		{
		{829, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\829\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\829\\Src\\", {{42,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\829\\Src\\FSRENAME14\x65B0\x65B0.A\x65B0", 
		}
		},
		
		{
		{830, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\830\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\830\\Src\\", {{43,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\830\\Src\\FSRENAME15\x65B0\x65B0.\x65B0", 
		}
		},  
		
		{
		{831, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\831\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\831\\Src\\", {{44,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\831\\Src\\RENAME\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		}
		},
		
		{
		{832, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\832\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\832\\Src\\", {{45,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\832\\Src\\RENAME\x65B0.AB\x65B0\x65B0", 
		}
		},
		
		{
		{833, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\833\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\833\\Src\\", {{46,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\833\\Src\\RENAME\x65B0.A\x65B0\x6587", 
		}
		},

		{
		{834, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\834\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\834\\Src\\", {{47,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\834\\Src\\RENAME\x65B0.A\x65B0", 
		}
		},
		
		{
		{835, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\835\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\835\\Src\\", {{48,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\835\\Src\\RENAME\x65B0.\x65B0", 
		}
		},
		
		{
		{836, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\836\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\836\\Src\\", {{49,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\836\\Src\\NAME\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		}
		},
		
		{
		{837, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\837\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\837\\Src\\", {{50,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\837\\Src\\NAME\x65B0.AB\x65B0\x65B0", 
		}
		},
		
		{
		{838, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\838\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\838\\Src\\", {{51,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\838\\Src\\NAME\x65B0.A\x65B0\x6587", 
		}
		},
		
		{
		{839, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\839\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\839\\Src\\", {{52,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\839\\Src\\NAME\x65B0.A\x65B0", 
		}
		},
		
		{
		{840, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\840\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\840\\Src\\", {{53,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\840\\Src\\NAME\x65B0.\x65B0", 
		}
		},
		
		{
		{841, ERFsRename}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\841\\Src\\\x65B0\x6587(AB)\\\x65B0\x4EF6(ABCDEF)\x4EF6\x6587\\", 
		(TText*)L"?:\\T_FCSC\\841\\Src\\", {BLOCK22, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\841\\Src\\\x65B0\x6587(AB)\\\x65B0\x4EF6(RENAMED_ME)\x4EF6\x6587\\", 
		}
		}, 
				
//Cases for RFile::Rename() with Mixed characters:		
		{
		{842, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\842\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\842\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\842\\Src\\RENAME\x65B0\x65B0\x65B0\x65B0ME01.AB\x65B0\x65B0\x65B0\x65B0	", 
		}
		},
			
		{
		{843, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\843\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\843\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\843\\Src\\RENAME\x65B0\x65B0\x65B0\x65B0ME02.AB\x65B0\x65B0", 
		}
		},	
		
		{
		{844, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\844\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\844\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\844\\Src\\RENAME\x65B0\x65B0\x65B0\x65B0ME03.A\x65B0\x6587", 
		}
		},
		
		{
		{845, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\845\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\845\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\845\\Src\\RENAME\x65B0\x65B0\x65B0\x65B0ME04.A\x65B0", 
		}
		}, 
		
		{
		{846, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\846\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\846\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\846\\Src\\RENAME\x65B0\x65B0\x65B0\x65B0ME05.\x65B0", 
		}
		},
		
		{
		{847, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\847\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\847\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\847\\Src\\RENAME06(\x65B0\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		}
		},  
		
		{
		{848, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\848\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\848\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\848\\Src\\RENAME07\x65B0\x65B0\x65B0.AB\x65B0\x65B0", 
		}
		},
		
		{
		{849, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\849\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\849\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\849\\Src\\RENAME08\x65B0(\x65B0)\x65B0.A\x65B0\x6587", 
		}
		},
		
		{
		{850, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\850\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\850\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\850\\Src\\RENAME09(\x65B0\x65B0)\x65B0.A\x65B0", 
		}
		},
		
		{
		{851, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\851\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\851\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\851\\Src\\RENAME10(\x65B0\x65B0\x65B0).\x65B0", 
		}
		},
			
		{
		{852, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\852\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\852\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\852\\Src\\RENAME11(\x65B0)\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		}
		},	
		
		{
		{853, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\853\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\853\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\853\\Src\\RENAME12\x65B0(\x65B0).AB\x65B0\x65B0", 
		}
		},
		
		{
		{854, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\854\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\854\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\854\\Src\\RENAME13(\x65B0\x65B0).A\x65B0\x6587", 
		}
		}, 
		
		{
		{855, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\855\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\855\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\855\\Src\\RENAME14\x65B0(\x65B0).A\x65B0", 
		}
		},
		
		{
		{856, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\856\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\856\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\856\\Src\\RENAME15(\x65B0\x65B0).\x65B0", 
		}
		},  
		
		{
		{857, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\857\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\857\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\857\\Src\\NAME\x65B0ME.AB\x65B0\x65B0\x65B0\x65B0", 
		}
		},
		
		{
		{858, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\858\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\858\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\858\\Src\\NAME\x65B0ME.AB\x65B0\x65B0", 
		}
		},
		
		{
		{859, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\859\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\859\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\859\\Src\\NAME(\x65B0).A\x65B0\x6587", 
		}
		},

		{
		{860, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\860\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\860\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\860\\Src\\NAMEME\x65B0.A\x65B0", 
		}
		},
		
		{
		{861, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\861\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\861\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\861\\Src\\NA(ME\x65B0).\x65B0", 
		}
		},
		
		{
		{862, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\862\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\862\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\862\\Src\\AB(\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		}
		},
		
		{
		{863, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\863\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\863\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\863\\Src\\CD)\x65B0(.AB\x65B0\x65B0", 
		}
		},
		
		{
		{864, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\864\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\864\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\864\\Src\\XY\x65B0)A.A\x65B0\x6587", 
		}
		},
		
		{
		{865, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\865\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\865\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\865\\Src\\PQR\x65B0S.A\x65B0", 
		}
		},
		
		{
		{866, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\866\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\866\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\866\\Src\\NAME\x65B0.\x65B0", 
		}
		},
										
		{
		{867, ERenameFile}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\867\\Src\\AB\x65B0\x6587(CDEF).\x4EF6(AB)", 
		(TText*)L"?:\\T_FCSC\\867\\Src\\", {{75,76,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\867\\Src\\AB(\x65B0\x6587)CD\\RENAMED.\x6587TX\x65B0XT", 
		}
		},
		
//Cases for RFs::Replace() with Mixed characters:		
		{
		{868, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\868\\Src\\\x65B0(A).\x65B0", 
		(TText*)L"?:\\T_FCSC\\868\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\868\\Src\\RENAME\x65B0\x65B0\x65B0\x65B0ME01.AB\x65B0\x65B0\x65B0\x65B0	", 
		}
		},
			
		{
		{869, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\869\\Src\\\x65B0(A).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\869\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\869\\Src\\RENAME\x65B0\x65B0\x65B0\x65B0ME02.AB\x65B0\x65B0", 
		}
		},	
		
		{
		{870, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\870\\Src\\\x65B0(A).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\870\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\870\\Src\\RENAME\x65B0\x65B0\x65B0\x65B0ME03.A\x65B0\x6587", 
		}
		},
		
		{
		{871, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\871\\Src\\\x65B0(A).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\871\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\871\\Src\\RENAME\x65B0\x65B0\x65B0\x65B0ME04.A\x65B0", 
		}
		}, 
		
		{
		{872, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\872\\Src\\\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\872\\Src\\", {BLOCK07, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\872\\Src\\RENAME\x65B0\x65B0\x65B0\x65B0ME05.\x65B0", 
		}
		},
		
		{
		{873, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\873\\Src\\\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\873\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\873\\Src\\RENAME06(\x65B0\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		}
		},  
		
		{
		{874, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\874\\Src\\(AB)\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\874\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\874\\Src\\RENAME07\x65B0\x65B0\x65B0.AB\x65B0\x65B0", 
		}
		},
		
		{
		{875, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\875\\Src\\\x65B0(AB)\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\875\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\875\\Src\\RENAME08\x65B0(\x65B0)\x65B0.A\x65B0\x6587", 
		}
		},
		
		{
		{876, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\876\\Src\\\x65B0(\x65B0)AB.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\876\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\876\\Src\\RENAME09(\x65B0\x65B0)\x65B0.A\x65B0", 
		}
		},
		
		{
		{877, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\877\\Src\\\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\877\\Src\\", {BLOCK08, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\877\\Src\\RENAME10(\x65B0\x65B0\x65B0).\x65B0", 
		}
		},
			
		{
		{878, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\878\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\878\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\878\\Src\\RENAME11(\x65B0)\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		}
		},	
		
		{
		{879, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\879\\Src\\\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0", 
		(TText*)L"?:\\T_FCSC\\879\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\879\\Src\\RENAME12\x65B0(\x65B0).AB\x65B0\x65B0", 
		}
		},
		
		{
		{880, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\880\\Src\\AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\880\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\880\\Src\\RENAME13(\x65B0\x65B0).A\x65B0\x6587", 
		}
		}, 
		
		{
		{881, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\881\\Src\\CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\881\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\881\\Src\\RENAME14\x65B0(\x65B0).A\x65B0", 
		}
		},
		
		{
		{882, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\882\\Src\\\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\882\\Src\\", {BLOCK09, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\882\\Src\\RENAME15(\x65B0\x65B0).\x65B0", 
		}
		},  
		
		{
		{883, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\883\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\883\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\883\\Src\\NAME\x65B0ME.AB\x65B0\x65B0\x65B0\x65B0", 
		}
		},
		
		{
		{884, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\884\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\884\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\884\\Src\\NAME\x65B0ME.AB\x65B0\x65B0", 
		}
		},
		
		{
		{885, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\885\\Src\\\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\885\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\885\\Src\\NAME(\x65B0).A\x65B0\x6587", 
		}
		},

		{
		{886, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\886\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\886\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\886\\Src\\NAMEME\x65B0.A\x65B0", 
		}
		},
		
		{
		{887, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\887\\Src\\\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\887\\Src\\", {BLOCK10, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\887\\Src\\NA(ME\x65B0).\x65B0", 
		}
		},
		
		{
		{888, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\888\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0", 
		(TText*)L"?:\\T_FCSC\\888\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\888\\Src\\AB(\x65B0).AB\x65B0\x65B0\x65B0\x65B0", 
		}
		},
		
		{
		{889, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\889\\Src\\(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0", 
		(TText*)L"?:\\T_FCSC\\889\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\889\\Src\\CD)\x65B0(.AB\x65B0\x65B0", 
		}
		},
		
		{
		{890, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\890\\Src\\\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\890\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\890\\Src\\XY\x65B0)A.A\x65B0\x6587", 
		}
		},
		
		{
		{891, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\891\\Src\\\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\891\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\891\\Src\\PQR\x65B0S.A\x65B0", 
		}
		},
		
		{
		{892, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\892\\Src\\CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\892\\Src\\", {BLOCK11, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\892\\Src\\NAME\x65B0.\x65B0", 
		}
		},
										
		{
		{893, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\893\\Src\\AB\x65B0\x6587(CDEF).\x4EF6(AB)", 
		(TText*)L"?:\\T_FCSC\\893\\Src\\", {{75,76,EOB}, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\893\\Src\\AB(\x65B0\x6587)CD\\RENAMED.\x6587TX\x65B0XT", 
		}
		},	
		
		{
		{894, ERFsReplace}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\894\\Src\\\x65B0\x6587(AB)\\\x65B0\x4EF6(AB)\x4EF6\\\x4EF6\x4EF6(ABC)\x65B0\x65B0\x6587.\x65B0\x6587", 
		(TText*)L"?:\\T_FCSC\\894\\Src\\", {BLOCK22, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\894\\Src\\\x65B0\x6587(AB)\\\x65B0\x4EF6(AB)\x4EF6\\\x4EF6\x4EF6(ABCDE)\x65B0\x6587.\x4EF6\x65B0\x6587", 
		}
		}, 
		
//Caess to demonstarte the short name generation and retreiving the long name
		{
		{895, ELongShortConversion}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\895\\Src\\\x65B0\x6587\x65B0\x4EF6.TXT", 
		(TText*)L"?:\\T_FCSC\\895\\Src\\", {BLOCK15, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\895\\Src\\\x65B0\x6587\x65B0\x4EF6.TXT", 
		}
		},
		
		{
		{896, ELongShortConversion}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\896\\Src\\\x65B0\x6587\x65B0\x4EF6(A).TXT", 
		(TText*)L"?:\\T_FCSC\\896\\Src\\", {BLOCK15, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\896\\Src\\\x65B0\x6587\x65B0~1.TXT", 
		}
		},
		
		{
		{897, ELongShortConversion}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\897\\Src\\\x65B0\x6587\x65B0\x4EF6(B).TXT", 
		(TText*)L"?:\\T_FCSC\\897\\Src\\", {BLOCK15, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\897\\Src\\\x65B0\x6587\x65B0~2.TXT", 
		}
		},		
		
		{
		{898, ELongShortConversion}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\898\\Src\\\x65B0\x6587\x65B0\x4EF6(C).TXT", 
		(TText*)L"?:\\T_FCSC\\898\\Src\\", {BLOCK15, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\898\\Src\\\x65B0\x6587\x65B0~3.TXT", 
		}
		},
		
		{
		{899, ELongShortConversion}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\899\\Src\\\x65B0.TXT", 
		(TText*)L"?:\\T_FCSC\\899\\Src\\", {BLOCK16, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\899\\Src\\\x65B0.TXT", 
		}
		},
		
		{
		{900, ELongShortConversion}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\900\\Src\\\x65B0\x6587.TXTTXT", 
		(TText*)L"?:\\T_FCSC\\900\\Src\\", {BLOCK16, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\900\\Src\\\x65B0\x6587~1.TXT", 
		}
		},
		
		{
		{901, ELongShortConversion}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\901\\Src\\\x65B0\x6587\x4EF6.TX", 
		(TText*)L"?:\\T_FCSC\\901\\Src\\", {BLOCK16, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\901\\Src\\\x65B0\x6587\x4EF6.TX", 
		}
		},
				
		{
		{902,ELongShortConversion}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\902\\Src\\ABCDE.\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\902\\Src\\", {BLOCK16, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\902\\Src\\ABCDE~1.\x65B0", 
		}
		},

		{
		{903, ELongShortConversion}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\903\\Src\\ABCDEFG.\x65B0\x65B0\x65B0", 
		(TText*)L"?:\\T_FCSC\\903\\Src\\", {BLOCK16, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\903\\Src\\ABCDEF~1.\x65B0", 
		}
		},
		
		{
		{904, ELongShortConversion}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\904\\Src\\ABCD.\x65B0T", 
		(TText*)L"?:\\T_FCSC\\904\\Src\\", {BLOCK16, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\904\\Src\\ABCD.\x65B0T", 
		}
		},
		
		{
		{905, ELongShortConversion}, 
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\905\\Src\\ABCDE.T\x65B0", 
		(TText*)L"?:\\T_FCSC\\905\\Src\\", {BLOCK16, EMPTY},
		},
		
		{&gDriveToTest, (TText*)L"?:\\T_FCSC\\905\\Src\\ABCDE.T\x65B0", 
		}
		}, 
		
						
//End biary API test cases 	
		{{0}}
				
		};

#endif //(_DEBUG) || defined(_DEBUG_RELEASE)
#endif /*T_FATCHARSETCONV_CASES_H*/
